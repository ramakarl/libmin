cmake CMakeLists.txt \
    -B../build/libmin \
    -DBUILD_CUDA=false \
    -DBUILD_OPENSSL=false \
    -DBUILD_BCRYPT=false \
    -DBUILD_OPENGL=true \
    -DBUILD_GLEW=true

make -C../build/libmin

cmake --build ../build/libmin --target install
