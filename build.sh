if [ ! -d build ];
then
    echo "Making build directory"
    mkdir build
fi

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
echo cmake --build . --target all --config $1