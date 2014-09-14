/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/app/appevironment.h>

using namespace et;

Environment::Environment() :
	_appPath(et::applicationPath()), _appPackagePath(et::applicationPackagePath()),
	_dataFolder(et::applicationDataFolder())
{
}

void Environment::updateDocumentsFolder(const ApplicationIdentifier& i)
{
	_documentsFolder = addTrailingSlash(libraryBaseFolder() + i.companyName);
	if (!folderExists(_documentsFolder))
		createDirectory(_documentsFolder, true);

	ET_ASSERT(folderExists(_documentsFolder));

	_documentsFolder += addTrailingSlash(i.applicationName);
	if (!folderExists(_documentsFolder))
		createDirectory(_documentsFolder, true);

	ET_ASSERT(folderExists(_documentsFolder));
}
