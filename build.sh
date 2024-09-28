cmake CMakeLists.txt \
    -DCMAKE_CXX_FLAGS=-fstack-protector-strong \
    -B../build/libmin \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_CUDA=false \
    -DBUILD_OPENSSL=true \
    -DBUILD_BCRYPT=true \
    -DBUILD_OPENGL=true \
    -DBUILD_GLEW=true

make -C../build/libmin
