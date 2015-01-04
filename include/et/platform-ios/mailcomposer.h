/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <set>
#include <et/core/singleton.h>
#include <et/app/events.h>

namespace et
{
	class MailComposerPrivate;
    class MailComposer : public Singleton<MailComposer>
    {
    public:
		static bool canSendEmail();

    public:
        void composeEmail(const std::string& recepient,
			const std::string& title, const std::string& text);

		void attachFile(const std::string& attachmentFileName);

		void present();

	private:
		MailComposer();
        ~MailComposer();
		ET_SINGLETON_COPY_DENY(MailComposer);
		
	private:
		friend class MailComposerPrivate;
		
	private:
		ET_DECLARE_PIMPL(MailComposer, 32)
	};
	
	inline MailComposer& mailComposer()
		{ return MailComposer::instance(); }
}
