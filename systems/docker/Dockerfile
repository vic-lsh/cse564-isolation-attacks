FROM ubuntu:latest

# Install required packages
RUN apt-get update && \
    apt-get install -y time && \
    rm -rf /var/lib/apt/lists/*

# Set working directory to ensure script can access data.txt
WORKDIR /mnt/app

# Execute the script when the container starts
CMD ["/mnt/app/run.sh"]

