cmake CMakeLists.txt \
    -B../build/libmin \
    -DBUILD_OPENSSL=true \
    -DBUILD_BCRYPT=true \
    -DBUILD_OPENGL=false 
    -DBUILD_GLEW=false

make -C../build/libmin
