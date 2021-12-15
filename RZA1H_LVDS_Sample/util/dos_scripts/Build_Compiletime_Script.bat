echo Creating  ..\src\renesas\application\bin\sWebSite.bin
..\util\EmbedFS -l -g -x -i "..\src\renesas\application\WebSite\*" -o "..\src\renesas\application\bin" -f "fsWebSite.bin"
echo Creating  ..\src\renesas\application\bin\WebData.bin
..\util\EmbedFS -l -g -x -i "..\src\renesas\application\WebData\*" -o "..\src\renesas\application\bin" -f "fsWebData.bin"
echo Incrementing Build Version Information in file ..\src\renesas\application\inc\version.h
..\util\buildinc ..\src\renesas\application\inc\version.h APPLICATION_INFO_BUILD
echo Script complete