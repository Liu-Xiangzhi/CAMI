FROM anony0000/eval_env_base:latest
WORKDIR /cami
COPY . /cami/
RUN cmake -S . -B build
RUN cmake --build build -j $(nproc)
RUN cmake --build build --target install
CMD ["/bin/bash"]