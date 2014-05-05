/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <set>
#include <et/core/singleton.h>
#include <et/app/events.h>

namespace et
{
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
    
    class PurchasesManager : public Singleton<PurchasesManager>
    {
    public:
        typedef std::set<std::string> ProductsSet;
		
		static bool purchasesEnabled();
		
		static const std::string defaultVerificationServer;
		static const std::string defaultVerificationSandboxServer;
        
    public:
		void setShouldVerifyReceipts(bool, const std::string&);
		
        void checkAvailableProducts(const ProductsSet& products);
		void restorePurchases();
		
        bool purchaseProduct(const std::string& product);
		
		PurchaseInfo purchaseInfoForProduct(const std::string&);
		
	public:
		ET_DECLARE_EVENT1(availableProductsChecked, StringList)
		ET_DECLARE_EVENT0(failedToCheckAvailableProducts)

		ET_DECLARE_EVENT1(productPurchased, std::string)
		ET_DECLARE_EVENT1(failedToPurchaseProduct, std::string)
		
		ET_DECLARE_EVENT1(purchaseRestored, std::string)
		ET_DECLARE_EVENT0(restoringPurchasesFinished)
		ET_DECLARE_EVENT0(failedToRestorePurchases)
		
	private:
		PurchasesManager();
		ET_SINGLETON_COPY_DENY(PurchasesManager)
    };
}
