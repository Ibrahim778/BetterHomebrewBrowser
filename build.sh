cd resource
cd rco
cd src
cd locale

for filename in ./*.xml; do
    python2 ../../cxml/appinfo/rcs_compiler.py -o ${filename%.*}.rcs $filename
done

cd ..
cd ..
cd ..

python2 rco/cxml/appinfo/appinfo_compiler.py -o bhbb_plugin.rco rco/src/bhbb_plugin.xml

cd ..

cd build

cmake ..
cmake --build . --target all --config debug
exit 1
move BetterHomebrewBrowser.vpk BetterHomebrewBrowser.zip
rmdir /S /Q BHBB00001
powershell -Command "Expand-Archive -Path ./BetterHomebrewBrowser.zip -DestinationPath ./BHBB00001 -Force
del BetterHomebrewBrowser.zip
cd ..

<NUL set /p=destroy| nc 192.168.137.27 1338
ncftpput -P 1337 -p b0ss -u Anonymous -R 192.168.137.27 /ux0:/app/ build\BHBB00001\
<NUL set /p=launch BHBB00001| nc 192.168.137.27 1338