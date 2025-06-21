FROM debian:stable-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
RUN mkdir /app
RUN mkdir /app/node
WORKDIR /app/node

# Copy source code
COPY ./node .
COPY ./common ../common

# Build the server
RUN make
# RUN make && ls -lR /app


EXPOSE 4224

ENTRYPOINT [ "/app/node/bin/main" ]
CMD [ "-V 5" ]