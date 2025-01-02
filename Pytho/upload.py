import psycopg2
import serial
import time
import webbrowser
import os
import openai
from dotenv import load_dotenv
from flask import Flask, request, jsonify
from threading import Thread

# Importing the necessary langchain modules
from langchain_community.llms import OpenAI
from langchain.vectorstores import Chroma
from langchain.embeddings import OpenAIEmbeddings
from langchain.prompts import ChatPromptTemplate
from langchain.chains import create_retrieval_chain
from langchain.chains.combine_documents import create_stuff_documents_chain
from langchain.schema import Document
from langchain.text_splitter import RecursiveCharacterTextSplitter



# Load environment variables from .env file
load_dotenv()

# Get OpenAI API key from environment variable
OPENAI_API_KEY = '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'

# Ensure the OpenAI API key is loaded
if not OPENAI_API_KEY:
    raise ValueError("OPENAI_API_KEY environment variable is not set")

# Define path to folder containing sensor data and instructions
data_folder = r"/home/slogiker/Desktop/CanSat/"
sensor_data_file = os.path.join(data_folder, "data.txt")
instructions_file = os.path.join(data_folder, "instructions.txt")

# Initialize language model (ChatGPT-4) using OpenAI API key
llm = OpenAI(model="gpt-4", openai_api_key=OPENAI_API_KEY)

# Function to load text files and return content as documents
def load_text_files(files):
    docs = []
    for file_path in files:
        with open(file_path, 'r') as file:
            text = file.read()
            docs.append(Document(page_content=text, metadata={"source": file_path}))
    return docs

# Load documents
all_docs = load_text_files([sensor_data_file, instructions_file])

# Split documents into smaller chunks for easier processing
text_splitter = RecursiveCharacterTextSplitter(chunk_size=1000, chunk_overlap=200)
splits = text_splitter.split_documents(all_docs)

# Create vector store from split documents using OpenAI embeddings
vectorstore = Chroma.from_documents(documents=splits, embedding=OpenAIEmbeddings(openai_api_key=OPENAI_API_KEY))

# Create retriever to retrieve information from vector store
retriever = vectorstore.as_retriever()

# Define system prompt
system_prompt = (
    "You are an AI assistant for the ESA project, helping to analyze and interpret sensor data collected during and after the flight of a satellite. "
    "Your tasks include analyzing flight data such as temperatures, pressures, accelerations, altitudes, and other measurements, and providing interpretations of this data.\n"
    "Identify possible anomalies or outliers in the data and suggest possible causes or solutions. "
    "Prepare reports and visualizations for easier understanding of the collected data. "
    "Advise on optimizations or changes for future flights based on the collected data. "
    "Answer specific questions from the team regarding flight results, technical details, and other project-related information.\n"
    "Use all available data and resources to provide the best possible answers. Be always professional, accurate, and helpful."
    "{context}"
)

# Define chat prompt template
prompt = ChatPromptTemplate.from_messages(
    [
        ("system", system_prompt),
        ("human", "{input}"),
    ]
)

# Create chain for question answering
question_answer_chain = create_stuff_documents_chain(llm, prompt)
rag_chain = create_retrieval_chain(retriever, question_answer_chain)

# Flask application
app = Flask(__name__)

@app.route('/api/chat', methods=['POST'])
def chat():
    user_input = request.json.get('input')
    results = rag_chain.invoke({"input": user_input})
    return jsonify({'answer': results['answer']})

def start_flask_app():
    app.run(port=5000, debug=True)

# Start Flask server in a separate thread
flask_thread = Thread(target=start_flask_app)
flask_thread.start()

# PostgreSQL database configuration
DB_NAME = 'cansat'
DB_USER = 'slogiker'
DB_PASSWORD = 'Plibersek.1'
DB_HOST = 'localhost'

# Serial port configuration
SERIAL_PORT = '/dev/ttyACM0'  # Replace with your serial port
BAUD_RATE = 9600

# Path to the PHP file
PHP_FILE_PATH = '/home/slogiker/Desktop/CanSat/index.php'

# Function to start the PHP server and open the index.php
def start_php_server():
    # Open the index.php file in the default web browser
    webbrowser.open_new_tab('http://localhost:80/index.php')

# Start the PHP server
start_php_server()
time.sleep(2)  # Give the server a moment to start

# Connect to the PostgreSQL database
conn = psycopg2.connect(
    dbname=DB_NAME,
    user=DB_USER,
    password=DB_PASSWORD,
    host=DB_HOST
)
cur = conn.cursor()

# Create table if it doesn't exist
cur.execute("""
    CREATE TABLE IF NOT EXISTS sensor_data (
        id SERIAL PRIMARY KEY,
        temperature NUMERIC(5,2),
        humidity NUMERIC(5,2),
        airpressure NUMERIC(5,2),
        batterypower NUMERIC(5,2),
        airquality NUMERIC(5,2),
        gyro_x NUMERIC(5,2),
        gyro_y NUMERIC(5,2),
        gyro_z NUMERIC(5,2),
        accel_x NUMERIC(5,2),
        accel_y NUMERIC(5,2),
        accel_z NUMERIC(5,2),
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
""")
conn.commit()

# Open the serial port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
time.sleep(2)  # Wait for the serial connection to initialize

# Loop to continuously check for data from the serial port
try:
    while True:
        try:
            if ser.in_waiting > 0:
                # Read a line from the serial port
                line = ser.readline().decode('utf-8').strip()
                # Split the line into individual sensor values
                values = line.split(" | ")
                if len(values) == 11:
                    # Insert the data into the database
                    cur.execute("""
                        INSERT INTO sensor_data (temperature, humidity, airpressure, batterypower, airquality, 
                                                 gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z)
                        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                    """, tuple(values))
                    conn.commit()
                    print(f"Inserted data: {values}")
            else:
                # If no data, print a message and continue
                time.sleep(1)
        except Exception as e:
            # If there's any error reading the data, print it and continue
            print(f"Error: {e}")
            time.sleep(1)
except KeyboardInterrupt:
    # Close the serial port and database connection on exit
    ser.close()
    cur.close()
    conn.close()
    print("Program terminated.")
