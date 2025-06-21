# Use a minimal base image with build tools
FROM debian:stable-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app/main

# Copy source code
COPY ./main-server .
COPY ../common ../common

# Build the server
RUN make normal

# Expose the TCP port
EXPOSE 4225

# Set default command

ENTRYPOINT [ "./bin/main" ]
CMD ["-V 5"]
