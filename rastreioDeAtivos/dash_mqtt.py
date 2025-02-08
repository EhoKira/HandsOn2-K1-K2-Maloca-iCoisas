import sqlite3
import paho.mqtt.client as mqtt
import streamlit as st
import json
import pandas as pd

# Configuração do MQTT para Broker Privado
MQTT_BROKER = "URL"  # Substitua pelo seu broker privado
MQTT_PORT = 8883    # Porta para SSL/TLS
MQTT_USERNAME = "USER"  # Se o broker exigir autenticação
MQTT_PASSWORD = "PASSWORD"  # Se o broker exigir autenticação
MQTT_TOPIC_CADASTRAR = "ativos/cadastrar"
MQTT_TOPIC_ATUALIZAR = "ativos/atualizar"

# Configuração do Streamlit
st.set_page_config(page_title="Monitoramento de Ativos", layout="wide")
st.title("Monitoramento de Ativos Hospitalares")

# Função para conectar ao banco de dados SQLite
def connect_db():
    return sqlite3.connect("ativos_hospital.db")

# Função para consultar todos os ativos do banco de dados
def get_ativos():
    conn = connect_db()
    cursor = conn.cursor()
    cursor.execute("SELECT id, tag, descricao, localizacao, data_entrada FROM ativos")
    ativos = cursor.fetchall()
    conn.close()
    return ativos

# Função de callback do MQTT para tratar eventos de conexão
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        st.success("Conectado com sucesso ao broker MQTT")
        # Após conectar, assinamos os tópicos
        client.subscribe(MQTT_TOPIC_CADASTRAR)
        client.subscribe(MQTT_TOPIC_ATUALIZAR)
        st.info(f"Assinado aos tópicos: {MQTT_TOPIC_CADASTRAR} e {MQTT_TOPIC_ATUALIZAR}")
    else:
        st.error(f"Falha na conexão, código de retorno: {rc}")

# Função de callback do MQTT para quando a mensagem for recebida
def on_message(client, userdata, msg):
    payload = json.loads(msg.payload.decode())
    st.write(f"Mensagem recebida no tópico: {msg.topic}")
    st.write(payload)

# Função para publicar mensagem no MQTT
def publicar_mqtt(topico, mensagem):
    # Criar um novo cliente MQTT
    client = mqtt.Client(client_id="", clean_session=True)  # Adicione client_id ou clean_session se necessário
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)  # Definindo usuário e senha
    
    # Configuração SSL/TLS
    client.tls_set()  # Usando certificado padrão (se necessário, forneça ca_certs, certfile e keyfile)

    client.on_connect = on_connect
    client.on_message = on_message

    # Conectar ao broker MQTT com SSL/TLS
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()  # Inicia o loop para tratar as mensagens recebidas e a conexão

        # Publicar a mensagem no tópico
        client.publish(topico, json.dumps(mensagem))
        st.success("Mensagem publicada com sucesso no MQTT!")

    except Exception as e:
        st.error(f"Erro ao conectar ao broker MQTT: {e}")

# Secao para adicionar ativo
with st.sidebar:
    st.header("Adicionar Ativo")
    tag = st.text_input("Código RFID (Tag)")
    descricao = st.text_input("Descrição")
    localizacao = st.text_input("Localização")
    
    if st.button("Cadastrar Ativo"):
        if tag and descricao and localizacao:
            dados = {
                "tag": tag,
                "descricao": descricao,
                "localizacao": localizacao
            }
            publicar_mqtt(MQTT_TOPIC_CADASTRAR, dados)
            st.success("Ativo cadastrado com sucesso!")
        else:
            st.warning("Preencha todos os campos!")

# Secao para atualizar localização ou descrição do ativo
with st.sidebar:
    st.header("Atualizar Localização ou Descrição de Ativo")
    tag_atualizar = st.text_input("Código RFID (Tag) do Ativo")
    nova_descricao = st.text_input("Nova Descrição")
    nova_localizacao = st.text_input("Nova Localização")
    
    if st.button("Atualizar Ativo"):
        if tag_atualizar:
            dados_atualizacao = {}
            if nova_localizacao:
                dados_atualizacao["localizacao"] = nova_localizacao
            if nova_descricao:
                dados_atualizacao["descricao"] = nova_descricao

            if dados_atualizacao:
                dados_atualizacao["tag"] = tag_atualizar
                publicar_mqtt(MQTT_TOPIC_ATUALIZAR, dados_atualizacao)
                st.success("Ativo atualizado com sucesso!")
            else:
                st.warning("Pelo menos um campo precisa ser preenchido para atualizar!")
        else:
            st.warning("Preencha o campo de Tag!")

# Exibindo todos os ativos cadastrados no banco de dados como uma tabela
st.header("Ativos Cadastrados")

# Consultar os dados dos ativos
ativos = get_ativos()

if ativos:
    # Convertendo os dados em um DataFrame do Pandas para exibição
    df_ativos = pd.DataFrame(ativos, columns=["ID", "Tag", "Descrição", "Localização", "Data de Entrada"])

    # Exibindo a tabela no Streamlit
    st.dataframe(df_ativos)  # exibe a tabela de maneira interativa
else:
    st.warning("Nenhum ativo cadastrado.")
