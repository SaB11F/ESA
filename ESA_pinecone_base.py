import openai
import gradio as gr
import os
import pinecone
from dotenv import load_dotenv
from langchain_community.document_loaders import PyPDFLoader
from langchain_community.embeddings import OpenAIEmbeddings
from langchain_text_splitters import RecursiveCharacterTextSplitter
from langchain_openai import ChatOpenAI
from langchain.chains import create_retrieval_chain
from langchain.chains.combine_documents import create_stuff_documents_chain
from langchain_core.prompts import ChatPromptTemplate
from pinecone import Pinecone
from langchain.vectorstores import Pinecone as LangchainPinecone

# Load environment variables from .env file
load_dotenv()

# Get API keys from environment variables
OPENAI_API_KEY = os.getenv('OPENAI_API_KEY')
PINECONE_API_KEY = os.getenv('PINECONE_API_KEY')

# Initialize Pinecone
pc = Pinecone(api_key=PINECONE_API_KEY)

# Connect to your existing Pinecone index
index_name = "esa-index"  # Replace with your actual index name
index = pc.Index(index_name)

# Initialize the embedding model
embed_model = OpenAIEmbeddings(openai_api_key=OPENAI_API_KEY)

# Create a Langchain vectorstore from the Pinecone index
vectorstore = LangchainPinecone(index, embed_model.embed_query, "text")

# Initialize the language model (ChatGPT-4) using the OpenAI API key
llm = ChatOpenAI(model="gpt-4o-mini", openai_api_key=OPENAI_API_KEY)

# Create a retriever from the vectorstore
retriever = vectorstore.as_retriever()

# Define the system prompt
system_prompt = (
    "Ti si AI asistent za projekt ESA, kjer s satelitom opremljenim s senzorji zbiramo podatke med poletom in po poletu. "
    "Tvoja naloga je, da pomagaš ekipi pri analiziranju in interpretaciji teh podatkov. S svojo pomočjo moraš zagotavljati natančne, jasne in koristne informacije o meritvah in podatkih zbranih med poletom.\n"
    "Tvoje naloge vključujejo:\n"
    "1. Analiziranje podatkov iz poleta, kot so temperature, tlaki, pospeški, višine in druge meritve, ter podajanje interpretacij teh podatkov.\n"
    "2. Identificiranje morebitnih anomalij ali izstopajočih vrednosti v podatkih in predlaganje možnih vzrokov ali rešitev.\n"
    "3. Priprava poročil in vizualizacij za lažje razumevanje zbranih podatkov.\n"
    "4. Svetovanje glede optimizacij ali sprememb za prihodnje polete na podlagi zbranih podatkov.\n"
    "5. Odgovarjanje na specifična vprašanja ekipe glede rezultatov poleta, tehničnih podrobnosti in drugih informacij povezanih s projektom.\n"
    "Uporabi vse razpoložljive podatke in vire, da zagotoviš najboljše možne odgovore. Bodi vedno strokoven, natančen in koristen."
    "{context}"
)

# Define the chat template
prompt = ChatPromptTemplate.from_messages(
    [
        ("system", system_prompt),
        ("human", "{input}"),
    ]
)

# Create a chain for question answering
question_answer_chain = create_stuff_documents_chain(llm, prompt)
rag_chain = create_retrieval_chain(retriever, question_answer_chain)

# Function for interacting with the chatbot via Gradio interface
def gradio_interface(user_input):
    results = rag_chain.invoke({"input": user_input})
    print("#######################")
    print(results)  # For easier debugging, you can remove this in production
    print("#######################")
    return results['answer']

# Set up the Gradio user interface
demo = gr.Interface(fn=gradio_interface, inputs="text", outputs="text", title="ESA AI")

# Launch the Gradio application and enable sharing via the web
demo.launch(share=True)