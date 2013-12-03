/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <set>
#include <et/core/et.h>

namespace et
{
    class PurchasesManagerDelegate
    {
    public:
        virtual ~PurchasesManagerDelegate() { }
        
        virtual void purchasesManagerDidCheckAvailableProducts(const StringList&) { }
        virtual void purchasesManagerDidFailToCheckAvailableProducts() { }
        
        virtual void purchasesManagerDidPurchaseProduct(const std::string&) { }
        virtual void purchasesManagerDidFailToPurchaseProduct(const std::string&) { }
        
        virtual void purchasesManagerDidRestorePurchasedProduct(const std::string&) { }
        virtual void purchasesManagerDidFinishRestoringPurchases() { }
        virtual void purchasesManagerDidFailToRestorePurchases() { }
    };
	
	struct PurchaseInfo
	{
		std::string description;
		std::string title;
		std::string identifier;
		std::string currency;
		std::string currencySymbol;
		
		float price;
		
		PurchaseInfo() :
			price(0.0f) { }
	};
    
    class PurchasesManager
    {
    public:
        typedef std::set<std::string> ProductsSet;
        
    public:
        void checkAvailableProducts(const ProductsSet& products, PurchasesManagerDelegate* delegate);
		void restoreTransactions(PurchasesManagerDelegate* delegate);
		
        bool purchaseProduct(const std::string& product, PurchasesManagerDelegate* delegate);
		
		PurchaseInfo purchaseInfoForProduct(const std::string&);
    };
}
