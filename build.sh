cmake CMakeLists.txt \
    -B../build/libmin \
    -DBUILD_CUDA=false \
    -DBUILD_OPENSSL=true \
    -DBUILD_BCRYPT=true \
    -DBUILD_OPENGL=true \
    -DBUILD_GLEW=true

make -C../build/libmin
