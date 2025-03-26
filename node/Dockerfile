FROM debian:latest

# set workdir (needed for relative path to user_objects)
WORKDIR /app

# create dirs
RUN mkdir -p /app/bin
RUN mkdir -p /app/user_objects

# copy bin
COPY ./bin /app/bin

# REMOVE ====
COPY ./user_objects /app/user_objects
#============

# set permission just in case
RUN chmod +x /app/bin/main

CMD ["/app/bin/main", "-p 12345", "-V 5"]