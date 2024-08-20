import openai
import gradio as gr
import os
from dotenv import load_dotenv
from langchain_community.document_loaders import PyPDFLoader
from langchain_community.vectorstores import Chroma
from langchain_community.embeddings import OpenAIEmbeddings
from langchain_text_splitters import RecursiveCharacterTextSplitter
from langchain_openai import ChatOpenAI
from langchain.chains import create_retrieval_chain
from langchain.chains.combine_documents import create_stuff_documents_chain
from langchain_core.prompts import ChatPromptTemplate

# Naloži okoljske spremenljivke iz .env datoteke
load_dotenv()

# Pridobi OpenAI API ključ iz okoljske spremenljivke
OPENAI_API_KEY = os.getenv('OPENAI_API_KEY')

# Definiranje poti do mape, kjer so shranjeni PDF dokumenti
file_path = r"" #dodaj svojo datoteko s podatki

# Pridobi vse PDF datoteke iz določene mape
pdf_files = [os.path.join(file_path, f) for f in os.listdir(file_path) if f.endswith(".pdf")]

all_docs = []

# Naloži vse PDF datoteke in jih shrani v seznam
for file_path in pdf_files:
    loader = PyPDFLoader(file_path)
    docs = loader.load()
    all_docs.extend(docs)

print(f"Total documents loaded: {len(all_docs)}")

# Inicializiraj jezikovni model (ChatGPT-4) z uporabo OpenAI API ključa
llm = ChatOpenAI(model="gpt-4", openai_api_key=OPENAI_API_KEY)

# Razdeli dokumente v manjše koščke za lažjo obdelavo
text_splitter = RecursiveCharacterTextSplitter(chunk_size=1000, chunk_overlap=200)
splits = text_splitter.split_documents(all_docs)

# Ustvari vektorsko shrambo iz razdeljenih dokumentov z uporabo OpenAI-jevih vektorjev
vectorstore = Chroma.from_documents(documents=splits, embedding=OpenAIEmbeddings(openai_api_key=OPENAI_API_KEY))

# Ustvari iskalni mehanizem za pridobivanje informacij iz vektorske shrambe
retriever = vectorstore.as_retriever()

# Definiranje sistemskega poziva, ki usmerja, kako naj AI odgovarja na vprašanja
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

# Definiraj predlogo za pogovor, kjer "človeški" uporabnik postavlja vprašanja in AI odgovarja
prompt = ChatPromptTemplate.from_messages(
    [
        ("system", system_prompt),
        ("human", "{input}"),
    ]
)

# Ustvari verigo, ki omogoča iskanje informacij v dokumentih in pripravo odgovorov
question_answer_chain = create_stuff_documents_chain(llm, prompt)
rag_chain = create_retrieval_chain(retriever, question_answer_chain)

# Funkcija za interakcijo s chatbotom prek Gradio vmesnika
def gradio_interface(user_input):
    results = rag_chain.invoke({"input": user_input})
    print("#######################")
    print(results)  # Za lažje debugiranje, lahko to odstranite v produkciji
    print("#######################")
    return results['answer']

# Nastavitev Gradio uporabniškega vmesnika
demo = gr.Interface(fn=gradio_interface, inputs="text", outputs="text", title="ESA Ai")

# Zaženi Gradio aplikacijo in omogoči deljenje prek spleta
demo.launch(share=True)
