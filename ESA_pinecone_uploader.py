import openai
import os
from dotenv import load_dotenv
from langchain_community.document_loaders import PyPDFLoader
from langchain_community.embeddings import OpenAIEmbeddings
from langchain_text_splitters import RecursiveCharacterTextSplitter
import pinecone
from pinecone import Pinecone

from pinecone import Pinecone

# Load environment variables from .env file
load_dotenv()

# Get OpenAI API key from environment variable
OPENAI_API_KEY = os.getenv('OPENAI_API_KEY')

# Get Pinecone API key from environment variable
PINECONE_API_KEY = os.getenv('PINECONE_API_KEY')

# Initialize Pinecone
pc = Pinecone(api_key=PINECONE_API_KEY)

# Define path to the PDF document
file_path = r"C:\ESA\Data_File\a11_missionreport.pdf"

# Load the PDF file
loader = PyPDFLoader(file_path)
docs = loader.load()

print(f"Total documents loaded: {len(docs)}")

# Initialize the embedding model
embedding_model = OpenAIEmbeddings(openai_api_key=OPENAI_API_KEY)

# Split documents into smaller chunks for better processing
text_splitter = RecursiveCharacterTextSplitter(chunk_size=1000, chunk_overlap=200)
splits = text_splitter.split_documents(docs)

# Create a Pinecone index
index_name = "esa-index"  # Define your index name
if index_name not in pc.list_indexes().names():
    pc.create_index(
        name=index_name,
        dimension=1536,  # OpenAI embeddings are 1536 dimensions
        metric='cosine'
    )

# Connect to the Pinecone index
index = pc.Index(index_name)

# Embed documents and upload to Pinecone
for i, doc in enumerate(splits):
    # Embed the document text
    embedding = embedding_model.embed_query(doc.page_content)
    
    # Upload the vector to Pinecone with a unique ID
    index.upsert(vectors=[(f"doc_{i}", embedding, {"source": doc.metadata['source'], "page": doc.metadata['page']})])

print(f"Uploaded {len(splits)} document chunks to Pinecone index '{index_name}'")