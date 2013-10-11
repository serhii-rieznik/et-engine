/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <StoreKit/StoreKit.h>
#include <et/platform-ios/iap.h>

using namespace et;

/*
 * ObjC wrapper
 */

@interface ObjCPurchasesManager : NSObject

+ (ObjCPurchasesManager*)sharedManager;
+ (BOOL)purchasesEnabled;

- (void)checkAvailableProducts:(NSSet*)products delegate:(PurchasesManagerDelegate*)delegate;
- (BOOL)purchaseProduct:(NSString*)product delegate:(PurchasesManagerDelegate*)delegate;
- (void)restorePurchasesWithDelegate:(PurchasesManagerDelegate*)delegate;

- (void)verifyReceiptForProduct:(NSString*)product data:(NSData*)receipt delegate:(PurchasesManagerDelegate*)delegate;

@end

/*
 * Product Request Wrapper
 */
#pragma mark - Product Request Wrapper

@interface ProductRequest : NSObject<SKProductsRequestDelegate>
{
	SKProductsRequest* _request;
	PurchasesManagerDelegate* _delegate;
}

- (id)initWithDelegate:(et::PurchasesManagerDelegate*)delegate productIdentifiers:(NSSet*)products;

@end

/*
 * Payment Wrapper
 */
#pragma mark - Payment Wrapper

@interface Payment : NSObject<SKPaymentTransactionObserver> 
{
	PurchasesManagerDelegate* _delegate;
	NSString* _productIdentifier;
}

- (id)initWithDelegateAndRestorePurchases:(et::PurchasesManagerDelegate*)delegate;
- (id)initWithDelegate:(et::PurchasesManagerDelegate*)delegate product:(SKProduct*)product;

@end

/*
 * Purchases Manager private methods
 */
#pragma mark - Purchases Manager private methods

@interface ObjCPurchasesManager() 
{
	NSMutableArray* _requests;
	NSMutableArray* _payments;
	NSMutableArray* _availableProducts;
}

- (void)addAvailableProduct:(SKProduct*)product;

- (void)removeRequest:(ProductRequest*)req;
- (void)removePayment:(Payment*)p;

- (void)internalVerityReciept:(NSDictionary*)objects;
- (void)internalRecieptVerified:(NSDictionary*)objects;

@end

/*
 * @implementation ObjCPurchasesManager
 */
#pragma mark - @implementation ObjCPurchasesManager

@implementation ObjCPurchasesManager

static ObjCPurchasesManager* sharedInstance = nil;

+ (ObjCPurchasesManager*)sharedManager
{
	if (sharedInstance == nil)
		sharedInstance = [[ObjCPurchasesManager alloc] init];
	return sharedInstance;
}

+ (BOOL)purchasesEnabled
{
	return [SKPaymentQueue canMakePayments];
}

#if (!ET_OBJC_ARC_ENABLED)
- (id)retain { return self; }
- (oneway void) release { }
- (NSUInteger)retainCount { return LONG_MAX; }
#endif

- (id)init
{
	self = [super init];
	if (self)
	{
		_requests = [[NSMutableArray alloc] init];
		_payments = [[NSMutableArray alloc] init];
		_availableProducts = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)checkAvailableProducts:(NSSet*)products delegate:(PurchasesManagerDelegate*)delegate
{
	ProductRequest* req = [[ProductRequest alloc] initWithDelegate:delegate productIdentifiers:products]; 
	[_requests addObject:req];
	
#if (!ET_OBJC_ARC_ENABLED)
	[req release];
#endif
}

- (BOOL)purchaseProduct:(NSString*)product delegate:(PurchasesManagerDelegate*)delegate
{
    SKProduct* availableProduct = nil;
	BOOL validPurchase = NO;
	for (SKProduct* p in _availableProducts)
	{
		if ([p.productIdentifier isEqualToString:product])
		{
            availableProduct = p;
			validPurchase = YES;
			break;
		}
	}
    
	if (validPurchase)
	{
		Payment* payment = [[Payment alloc] initWithDelegate:delegate product:availableProduct];
		[_payments addObject:payment];
#if (!ET_OBJC_ARC_ENABLED)
		[payment release];
#endif
	}
	
	return validPurchase;
}

- (void)restorePurchasesWithDelegate:(PurchasesManagerDelegate*)delegate
{
	Payment* p = [[Payment alloc] initWithDelegateAndRestorePurchases:delegate];
	[_payments addObject:p];
	
#if (!ET_OBJC_ARC_ENABLED)
	[p release];
#endif
}

- (void)removeRequest:(ProductRequest*)req
{
	[_requests removeObject:req];
}

- (void)removePayment:(Payment*)p
{
	[_payments removeObject:p];
}

- (void)addAvailableProduct:(SKProduct*)product
{
	[_availableProducts addObject:product];
}

- (void)internalVerityReciept:(NSDictionary*)objects
{
/*    
    NSString* product = [objects objectForKey:@"kProduct"];
    NSData* data = [objects objectForKey:@"kData"];
    NSObject* delegate = [objects objectForKey:@"kDelegate"];
    NSNumber* result = nil;
    
#if defined(ET_PRODUCTION_BUILD)
    NSString *urlsting = @"https://buy.itunes.apple.com/verifyReceipt";
#else    
    #warning ObjCPurchasesManager is connecting to the sandbox environment
    NSString* urlsting = @"https://sandbox.itunes.apple.com/verifyReceipt";
#endif    
    
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlsting]];
    NSString *json = [NSString stringWithFormat:@"{\"receipt-data\":\"%@\"}", 
                      [data base64EncodingWithLineLength:[data length]]];
    NSString* length = [NSString stringWithFormat:@"%d", [json length]];	
    
    [request setHTTPBody:[json dataUsingEncoding:NSUTF8StringEncoding]];
    [request setHTTPMethod:@"POST"];		
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setValue:length forHTTPHeaderField:@"Content-Length"];	
    
    NSError* requestError = nil;
    NSError* readingError = nil;
    NSHTTPURLResponse* requestResponse = nil;
    
    NSData* responseData = [NSURLConnection sendSynchronousRequest:request returningResponse:&requestResponse error:&requestError];  
    if ((responseData == nil) || (requestError != nil))
    {
        result = [NSNumber numberWithBool:NO];
    }
    else 
    {
        id response = [NSJSONSerialization JSONObjectWithData:responseData options:0 error:&readingError];
        if ((response == nil) || (readingError != nil) || ![response isKindOfClass:[NSDictionary class]])
        {
            result = [NSNumber numberWithBool:NO];
        }
        else 
        {
            NSNumber* status = [response objectForKey:@"status"];
            if (status == nil)
            {
                result = [NSNumber numberWithBool:NO];
            }
            else 
            {
                NSDictionary* receipt = [response objectForKey:@"receipt"];
                if (receipt == nil)
                {
                    result = [NSNumber numberWithBool:NO];
                }
                else 
                {
                    NSString* productId = [receipt objectForKey:@"product_id"];
                    if (productId == nil)
                    {
                        result = [NSNumber numberWithBool:NO];
                    }
                    else
                    {
                        BOOL sameProduct = [productId isEqualToString:product];
                        BOOL statusValidated = [status intValue] == 0;
                        result = [NSNumber numberWithBool:statusValidated && sameProduct];
                    }
                }
            }
        }
    }
    
    NSDictionary* resultObjects = [NSDictionary dictionaryWithObjectsAndKeys:product, @"kProduct", result, @"kResult", delegate, @"kDelegate", nil];
    [self performSelectorOnMainThread:@selector(internalRecieptVerified:) withObject:resultObjects waitUntilDone:YES];
*/ 
}

- (void)verifyReceiptForProduct:(NSString*)product data:(NSData*)receipt delegate:(PurchasesManagerDelegate*)delegate
{
/*    
    NSDictionary* objects = [NSDictionary dictionaryWithObjectsAndKeys:
                             product, @"kProduct", receipt, @"kData", 
                             [NSNumber numberWithUnsignedLongLong:reinterpret_cast<unsigned long long>(delegate)], @"kDelegate", nil];
    [self performSelectorInBackground:@selector(internalVerityReciept:) withObject:objects];
*/ 
}

- (void)internalRecieptVerified:(NSDictionary*)objects
{
/*    
    NSString* product = [objects objectForKey:@"kProduct"];
    NSNumber* result = [objects objectForKey:@"kResult"];
    PurchasesManagerDelegate* delegate = reinterpret_cast<PurchasesManagerDelegate*>([[objects objectForKey:@"kDelegate"] unsignedLongLongValue]);
    if ([delegate respondsToSelector:@selector(ObjCPurchasesManagerDidVerifyReceiptForProduct:successful:)])
        [delegate ObjCPurchasesManagerDidVerifyReceiptForProduct:product successful:[result boolValue]];
*/ 
}

@end

/*
 * @implementation ProductRequest
 */
#pragma mark - @implementation ProductRequest

@implementation ProductRequest

- (id)initWithDelegate:(PurchasesManagerDelegate*)delegate productIdentifiers:(NSSet*)products
{
	self = [super init];
	if (self)
	{
		_delegate = delegate;
		
		SKProductsRequest* req = [[SKProductsRequest alloc] initWithProductIdentifiers:products];
		req.delegate = self;
		[req start];
	}
	return self;
}

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    StringList checkedProducts;
    
	for (SKProduct* p in response.products)
    {
		[[ObjCPurchasesManager sharedManager] addAvailableProduct:p];
        checkedProducts.push_back([p.productIdentifier cStringUsingEncoding:NSASCIIStringEncoding]);
	}
    
    _delegate->purchasesManagerDidCheckAvailableProducts(checkedProducts);
}

- (void)requestDidFinish:(SKRequest *)request
{
	[[ObjCPurchasesManager sharedManager] removeRequest:self];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
    _delegate->purchasesManagerDidFailToCheckAvailableProducts();
	[[ObjCPurchasesManager sharedManager] removeRequest:self];
}

@end


/*
 * @implementation Payment
 */
#pragma mark - @implementation Payment

@implementation Payment

- (id)initWithDelegate:(PurchasesManagerDelegate*)delegate product:(SKProduct*)product
{
	self = [super init];
	if (self)
	{
		_delegate = delegate;
		_productIdentifier = product.productIdentifier;
		
#if (!ET_OBJC_ARC_ENABLED)
		[_productIdentifier retain];
#endif
		
		SKPayment* payment = [SKMutablePayment paymentWithProduct:product];
		
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		[[SKPaymentQueue defaultQueue] addPayment:payment];
	}
	return self;
}

- (id)initWithDelegateAndRestorePurchases:(PurchasesManagerDelegate*)delegate
{
	self = [super init];
	if (self)
	{
		_delegate = delegate;
		_productIdentifier = nil;
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
	}
	return self;
}

- (void)dealloc
{
	[[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
	
#if (!ET_OBJC_ARC_ENABLED)
	[_productIdentifier release];
	[super dealloc];
#endif
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
    NSLog(@"Updating transactions: %@", transactions);
    
	for (SKPaymentTransaction* t in transactions)
	{
		NSString* productId = t.payment.productIdentifier;
        const char* productIdcStr = [productId cStringUsingEncoding:NSASCIIStringEncoding];
		SKPaymentTransactionState state = t.transactionState;
		
		if ((_productIdentifier == nil) || [productId isEqualToString:_productIdentifier])
		{
			switch (state)
			{
				case SKPaymentTransactionStateRestored:
				{
                    NSLog(@"SKPaymentTransactionStateRestored: %@", productId);
                    _delegate->purchasesManagerDidRestorePurchasedProduct(productIdcStr);
					break;
				}
                    
				case SKPaymentTransactionStatePurchased:
				{
                    NSLog(@"SKPaymentTransactionStatePurchased: %@", productId);
                    _delegate->purchasesManagerDidPurchaseProduct(productIdcStr);
					break;
				}
					
				case SKPaymentTransactionStateFailed:
				{
                    NSLog(@"SKPaymentTransactionStateFailed: %@", productId);
                    _delegate->purchasesManagerDidFailToPurchaseProduct(productIdcStr);
					break;
				}
                    
				default:
					break;
			}
		}
		
		if (state != SKPaymentTransactionStatePurchasing)
		{
			NSLog(@"Finishing transaction: %@", _productIdentifier);
			[[SKPaymentQueue defaultQueue] finishTransaction:t];
		}
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
	_delegate->purchasesManagerDidFailToRestorePurchases();
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
	_delegate->purchasesManagerDidFinishRestoringPurchases();
}

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions
{
	[[ObjCPurchasesManager sharedManager] removePayment:self];
}

@end

/*
 * C++ stuff
 */

PurchasesManager::PurchasesManager()
{
}

PurchasesManager::~PurchasesManager()
{
}

void PurchasesManager::checkAvailableProducts(const ProductsSet& products, PurchasesManagerDelegate* delegate)
{
    NSMutableSet* objcSet = [NSMutableSet set];
    for (ProductsSet::const_iterator i = products.begin(), e = products.end(); i != e; ++i)
        [objcSet addObject:[NSString stringWithCString:i->c_str() encoding:NSASCIIStringEncoding]];
    
    [[ObjCPurchasesManager sharedManager] checkAvailableProducts:objcSet delegate:delegate];
}

bool PurchasesManager::purchaseProduct(const std::string& product, PurchasesManagerDelegate* delegate)
{
    NSString* objcString = [NSString stringWithCString:product.c_str() encoding:NSASCIIStringEncoding];
    return [[ObjCPurchasesManager sharedManager] purchaseProduct:objcString delegate:delegate];
}

void PurchasesManager::restoreTransactions(PurchasesManagerDelegate* delegate)
{
    return [[ObjCPurchasesManager sharedManager] restorePurchasesWithDelegate:delegate];
}