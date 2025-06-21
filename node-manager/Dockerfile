FROM debian:stable-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

    
WORKDIR /app/node
# common src code
COPY ../common ..
# compute node src code
COPY ../node .
# build compute node
RUN make normal


# node-manager
WORKDIR /app/node-manager
# node-manager src-code
COPY . .
# Build node manager
RUN make normal


EXPOSE 4224


# start node manager
CMD ["/app/node-manager/bin/main", "-V 5"]