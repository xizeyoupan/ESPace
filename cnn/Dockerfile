# Use official Python image as base
FROM python:3.11-slim

ARG HOST
ARG PORT

# Set working directory
WORKDIR /app

# Install PDM
RUN pip install pdm

# Copy project files
COPY . .

# Install dependencies using PDM
RUN pdm install

EXPOSE ${PORT}

# Set default command (adjust as needed)
CMD ["pdm", "run", "python", "app.py"]
