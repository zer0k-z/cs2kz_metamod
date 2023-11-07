FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk

WORKDIR /app
VOLUME /app/build

RUN apt update && apt install -y git python3-pip
RUN git clone https://github.com/alliedmodders/ambuild
RUN pip install ./ambuild

COPY . .
CMD [ "/bin/bash", "./docker-entrypoint.sh" ]
