#!/bin/bash
backupdir="eclipse-files"

targetdirs=("lz4" "bewalgo" "parallel" "interfaces" "memcpy")

for targetindex in "${!targetdirs[@]}"; do
	targetdir=${targetdirs[$targetindex]}
	rm -rf "${targetdir}/.settings"
	mkdir "${targetdir}/.settings"
	cat "${backupdir}/.cproject"				 | sed "s#ipcc-l-lz4#${targetdir}#g"	> "${targetdir}/.cproject"
	cat "${backupdir}/.project"				 | sed "s#ipcc-l-lz4#${targetdir}#g"	> "${targetdir}/.project"
	cat "${backupdir}/.settings/.gitignore"			 | sed "s#ipcc-l-lz4#${targetdir}#g"	> "${targetdir}/.settings/.gitignore"
	cat "${backupdir}/.settings/language.settings.xml"	 | sed "s#ipcc-l-lz4#${targetdir}#g"	> "${targetdir}/.settings/language.settings.xml"
done
