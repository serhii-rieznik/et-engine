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

- (void)validateTransaction:(SKPaymentTransaction*)transaction;

- (BOOL)parseValidationResponseFromBundle:(NSDictionary*)response forTransaction:(SKPaymentTransaction*)transaction;
- (BOOL)parseValidationResponseFromTransaction:(NSDictionary*)response forTransaction:(SKPaymentTransaction*)transaction;

@property (nonatomic, assign) BOOL shouldVerifyReceipts;

@end

const std::string PurchasesManager::defaultVerificationServer = "https://buy.itunes.apple.com/verifyReceipt";
const std::string PurchasesManager::defaultVerificationSandboxServer = "https://sandbox.itunes.apple.com/verifyReceipt";

PurchasesManager::PurchasesManager()
{
	
}

bool PurchasesManager::purchasesEnabled()
{
	return [SKPaymentQueue canMakePayments];
}

void PurchasesManager::setShouldVerifyReceipts(bool verify)
{
	[[SharedPurchasesManager sharedInstance] setShouldVerifyReceipts:verify ? YES : NO];
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

@synthesize shouldVerifyReceipts = _shouldVerifyReceipts;

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

- (BOOL)parseValidationResponseFromTransaction:(NSDictionary*)response forTransaction:(SKPaymentTransaction*)transaction
{
	/*
	{
		receipt =     {
			bid = "com.cloudgears.kubik";
			bvrs = "1.0";
			"item_id" = 871095982;
			"original_purchase_date" = "2014-05-05 16:11:01 Etc/GMT";
			"original_purchase_date_ms" = 1399306261316;
			"original_purchase_date_pst" = "2014-05-05 09:11:01 America/Los_Angeles";
			"original_transaction_id" = 1000000109714902;
			"product_id" = "com.cloudgears.kubik.coins.large";
			"purchase_date" = "2014-05-05 16:11:01 Etc/GMT";
			"purchase_date_ms" = 1399306261316;
			"purchase_date_pst" = "2014-05-05 09:11:01 America/Los_Angeles";
			quantity = 1;
			"transaction_id" = 1000000109714902;
			"unique_identifier" = fa3ec72a834764cd268ab65dad892ab9e36fd51b;
			"unique_vendor_identifier" = "911744A8-9229-4C27-8E0F-DE2FA414FE7C";
		};
		status = 0;
	}
	*/
	
	NSNumber* status = [response objectForKey:@"status"];
	if ((status == nil) || ([status integerValue] != 0))
		return NO;
	
	NSDictionary* receipt = [response objectForKey:@"receipt"];
	if (receipt == nil)
		return NO;
	
	NSString* bundleId = [receipt objectForKey:@"bid"];
	if (![bundleId isEqualToString:[[NSBundle mainBundle] bundleIdentifier]])
		return NO;
	
	NSString* productId = [receipt objectForKey:@"product_id"];
	if (![productId isEqualToString:transaction.payment.productIdentifier])
		return NO;
	
	NSString* transactionId = [receipt objectForKey:@"transaction_id"];
	if (![transactionId isEqualToString:transaction.transactionIdentifier])
		return NO;
	
	return YES;
}

- (BOOL)parseValidationResponseFromBundle:(NSDictionary*)response forTransaction:(SKPaymentTransaction*)transaction
{
	NSNumber* status = [response objectForKey:@"status"];
	if ((status == nil) || ([status integerValue] != 0))
		return NO;
	
	NSDictionary* receipt = [response objectForKey:@"receipt"];
	if (receipt == nil)
		return NO;
	
	NSString* bundleId = [receipt objectForKey:@"bundle_id"];
	if (![bundleId isEqualToString:[[NSBundle mainBundle] bundleIdentifier]])
		return NO;
	
	NSArray* iaps = [receipt objectForKey:@"in_app"];
	if (iaps == nil)
		return NO;
	
	for (NSDictionary* iap in iaps)
	{
		NSString* productId = [iap objectForKey:@"product_id"];
		if ([productId isEqualToString:transaction.payment.productIdentifier])
		{
			NSString* transactionId = [iap objectForKey:@"transaction_id"];
			if ([transactionId isEqualToString:transaction.transactionIdentifier])
				return YES;
		}
	}
	
	return NO;
}

- (void)validateTransaction:(SKPaymentTransaction*)transaction
{
	BOOL usingBundleReceipt = NO; // [[NSBundle mainBundle] respondsToSelector:@selector(appStoreReceiptURL)];
	
	NSData* receipt = usingBundleReceipt ?
		[NSData dataWithContentsOfURL:[[NSBundle mainBundle] appStoreReceiptURL]] : transaction.transactionReceipt;
	
	if (receipt == nil)
	{
		PurchasesManager::instance().failedToPurchaseProduct.invokeInMainRunLoop(
			std::string([transaction.payment.productIdentifier UTF8String]));
		return;
	}
		
	NSError* error = nil;
	NSDictionary* requestContents = @{ @"receipt-data" : [receipt base64EncodedStringWithOptions:0] };
	NSData* requestData = [NSJSONSerialization dataWithJSONObject:requestContents options:0 error:&error];
	
	[self verifyRequestData:requestData
		withServer:[NSString stringWithUTF8String:PurchasesManager::defaultVerificationServer.c_str()]
		transaction:transaction usingBundleReceipt:usingBundleReceipt];
}

-(void)verifyRequestData:(NSData*)requestData withServer:(NSString*)server
	transaction:(SKPaymentTransaction*)transaction usingBundleReceipt:(BOOL)usingBundleReceipt
{
	NSURL* url = [NSURL URLWithString:server];
	NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
	[request setHTTPMethod:@"POST"];
	[request setHTTPBody:requestData];
	
	NSOperationQueue* queue = [[NSOperationQueue alloc] init];
	[NSURLConnection sendAsynchronousRequest:request queue:queue
		completionHandler:^(NSURLResponse* response, NSData* data, NSError* connectionError)
	{
		std::string productId([transaction.payment.productIdentifier UTF8String]);
		
		if (connectionError == nil)
		{
			NSError* error = nil;
			NSDictionary* values = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
			
			if ([[values objectForKey:@"status"] integerValue] == 21007)
			{
				dispatch_async(dispatch_get_main_queue(), ^
				{
					[self verifyRequestData:requestData
						withServer:[NSString stringWithUTF8String:PurchasesManager::defaultVerificationSandboxServer.c_str()]
						transaction:transaction usingBundleReceipt:usingBundleReceipt];
				});
			}
			else if ((values != nil) && (error == nil))
			{
				BOOL validationPassed = usingBundleReceipt ?
					[self parseValidationResponseFromBundle:values forTransaction:transaction] :
					[self parseValidationResponseFromTransaction:values forTransaction:transaction];
				
				if (validationPassed)
					PurchasesManager::instance().productPurchased.invokeInMainRunLoop(productId);
				else
					PurchasesManager::instance().failedToPurchaseProduct.invokeInMainRunLoop(productId);
			}
			else
			{
				PurchasesManager::instance().failedToPurchaseProduct.invokeInMainRunLoop(productId);
			}
		}
		else
		{
			PurchasesManager::instance().failedToPurchaseProduct.invokeInMainRunLoop(productId);
		}
	}];
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
			if (_shouldVerifyReceipts)
			{
				[self validateTransaction:transaction];
			}
			else
			{
				PurchasesManager::instance().productPurchased.invokeInMainRunLoop(
					std::string([transaction.payment.productIdentifier UTF8String]));
			}
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
