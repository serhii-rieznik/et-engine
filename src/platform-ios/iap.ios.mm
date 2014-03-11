/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <StoreKit/StoreKit.h>
#include <et/platform-ios/iap.h>

using namespace et;

@interface SharedPurchasesManager : NSObject <SKPaymentTransactionObserver, SKProductsRequestDelegate>
{
	NSMutableDictionary* _availableProducts;
}

+ (instancetype)sharedInstance;

- (void)startProductRequestWithIdentifiers:(const PurchasesManager::ProductsSet&)products;

- (PurchaseInfo)purchaseInfoForIdentifier:(const std::string&)identifier;

- (BOOL)purchaseProduct:(const std::string&)identifier;

@end

PurchasesManager::PurchasesManager()
{
	
}

bool PurchasesManager::purchasesEnabled()
{
	return [SKPaymentQueue canMakePayments];
}

void PurchasesManager::checkAvailableProducts(const ProductsSet& products)
{
	[[SharedPurchasesManager sharedInstance] startProductRequestWithIdentifiers:products];
}

PurchaseInfo PurchasesManager::purchaseInfoForProduct(const std::string& pId)
{
	return [[SharedPurchasesManager sharedInstance] purchaseInfoForIdentifier:pId];
}

bool PurchasesManager::purchaseProduct(const std::string& product)
{
	return [[SharedPurchasesManager sharedInstance] purchaseProduct:product];
}

void PurchasesManager::restorePurchases()
{
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

/*
 * Obj-C implementation
 */
@implementation SharedPurchasesManager

+ (instancetype)sharedInstance
{
	static SharedPurchasesManager* _sharedInstance = nil;
	static dispatch_once_t onceToken = 0;
	dispatch_once(&onceToken, ^{
		_sharedInstance = [[SharedPurchasesManager alloc] init];
	});
	return _sharedInstance;
}

- (id)init
{
	self = [super init];
	if (self)
	{
		_availableProducts = [[NSMutableDictionary alloc] init];
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
	}
	return self;
}

- (void)startProductRequestWithIdentifiers:(const PurchasesManager::ProductsSet&)products
{
	NSMutableSet* identifiers = [NSMutableSet set];
	
	for (const auto& p : products)
		[identifiers addObject:[NSString stringWithUTF8String:p.c_str()]];
	
	SKProductsRequest* request = [[SKProductsRequest alloc] initWithProductIdentifiers:identifiers];
	[request setDelegate:self];
	[request start];
}

- (PurchaseInfo)purchaseInfoForIdentifier:(const std::string&)identifier
{
	PurchaseInfo result;
	SKProduct* product = [_availableProducts objectForKey:[NSString stringWithUTF8String:identifier.c_str()]];
	if (product != nil)
	{
		NSString* currency = [product.priceLocale objectForKey:NSLocaleCurrencyCode];
		NSString* currencySymbol = [product.priceLocale objectForKey:NSLocaleCurrencySymbol];
		
		result.identifier = [product.productIdentifier UTF8String];
		result.description = [product.localizedDescription UTF8String];
		result.title = [product.localizedTitle UTF8String];
		result.price = [product.price floatValue];
		result.currency = [currency UTF8String];
		result.currencySymbol = [currencySymbol UTF8String];
	}
	return result;
}

- (BOOL)purchaseProduct:(const std::string&)identifier
{
	SKProduct* product = [_availableProducts objectForKey:[NSString stringWithUTF8String:identifier.c_str()]];
	if (product == nil)
		return NO;

	[[SKPaymentQueue defaultQueue] addPayment:[SKPayment paymentWithProduct:product]];
	return YES;
}

/*
 * Product request delegate
 */
- (void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response
{
    StringList checkedProducts;
    
	for (SKProduct* p in response.products)
	{
		NSString* identifier = p.productIdentifier;
		[_availableProducts setObject:p forKey:identifier];
		checkedProducts.push_back(std::string([identifier UTF8String]));
	}
	
	PurchasesManager::instance().availableProductsChecked.invokeInMainRunLoop(checkedProducts);
}

- (void)request:(SKRequest*)request didFailWithError:(NSError *)error
{
	PurchasesManager::instance().failedToCheckAvailableProducts.invokeInMainRunLoop();
}

- (void)requestDidFinish:(SKRequest*)request
{
	
}

/*
 * Transaction observer delegate
 */
- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions
{
	NSMutableArray* transactionsToFinish = [NSMutableArray array];
	
	for (SKPaymentTransaction* transaction in transactions)
	{
		SKPaymentTransactionState state = transaction.transactionState;
		
		if (state == SKPaymentTransactionStatePurchased)
		{
			std::string productId([transaction.payment.productIdentifier UTF8String]);
			PurchasesManager::instance().productPurchased.invokeInMainRunLoop(productId);
		}
		else if (state == SKPaymentTransactionStateRestored)
		{
			std::string productId([transaction.originalTransaction.payment.productIdentifier UTF8String]);
			PurchasesManager::instance().purchaseRestored.invokeInMainRunLoop(productId);
		}
		else if (state == SKPaymentTransactionStateFailed)
		{
			std::string productId([transaction.payment.productIdentifier UTF8String]);
			PurchasesManager::instance().failedToPurchaseProduct.invokeInMainRunLoop(productId);
		}
		
		if (state != SKPaymentTransactionStatePurchasing)
			[transactionsToFinish addObject:transaction];
	}
	
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
	{
		for (SKPaymentTransaction* transaction in transactionsToFinish)
			[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
	});
}

- (void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error
{
	PurchasesManager::instance().failedToRestorePurchases.invokeInMainRunLoop();
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue
{
	PurchasesManager::instance().restoringPurchasesFinished.invokeInMainRunLoop();
}

@end
