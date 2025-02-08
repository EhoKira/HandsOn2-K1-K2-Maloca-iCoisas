import sqlite3
import paho.mqtt.client as mqtt
import json

# Configuração do MQTT para Broker Privado
MQTT_BROKER = "URL"  # Substitua pelo seu broker privado
MQTT_PORT = 8883    # Porta para SSL/TLS
MQTT_USERNAME = "USER"  # Se o broker exigir autenticação
MQTT_PASSWORD = "PASSWORD"  # Se o broker exigir autenticação
MQTT_TOPIC_CADASTRAR = "ativos/cadastrar"
MQTT_TOPIC_ATUALIZAR = "ativos/atualizar"

# Função para conectar ao banco de dados
def connect_db():
    return sqlite3.connect("ativos_hospital.db")

# Função para criar a tabela de ativos se não existir
def create_table():
    conn = connect_db()
    cursor = conn.cursor()
    cursor.execute(""" 
    CREATE TABLE IF NOT EXISTS ativos (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        tag TEXT UNIQUE NOT NULL,
        descricao TEXT,
        localizacao TEXT,
        data_entrada TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )""")
    conn.commit()
    conn.close()

create_table()

# Função de callback quando uma mensagem for recebida
def on_message(client, userdata, msg):
    payload = json.loads(msg.payload.decode())
    
    if msg.topic == MQTT_TOPIC_CADASTRAR:
        cadastrar_ativo(payload)
    elif msg.topic == MQTT_TOPIC_ATUALIZAR:
        atualizar_ativo(payload)

# Função para cadastrar ativo no banco de dados
def cadastrar_ativo(data):
    tag = data.get("tag")
    descricao = data.get("descricao", "Nao especificado")
    localizacao = data.get("localizacao", "Nao especificado")

    conn = connect_db()
    cursor = conn.cursor()
    try:
        cursor.execute("INSERT INTO ativos (tag, descricao, localizacao) VALUES (?, ?, ?)", (tag, descricao, localizacao))
        conn.commit()
        print(f"Ativo {tag} cadastrado com sucesso!")
    except sqlite3.IntegrityError:
        print(f"Erro: Tag {tag} já cadastrada!")
    finally:
        conn.close()

# Função para atualizar ativo no banco de dados
def atualizar_ativo(data):
    tag = data.get("tag")
    nova_localizacao = data.get("localizacao")
    nova_descricao = data.get("descricao")

    conn = connect_db()
    cursor = conn.cursor()

    # Verificar se o ativo existe no banco de dados
    cursor.execute("SELECT * FROM ativos WHERE tag = ?", (tag,))
    ativo = cursor.fetchone()

    if not ativo:
        print(f"Erro: Ativo {tag} não encontrado!")
        conn.close()
        return

    # Atualizar os campos que foram passados
    update_fields = []
    values = []

    if nova_localizacao:
        update_fields.append("localizacao = ?")
        values.append(nova_localizacao)
    
    if nova_descricao:
        update_fields.append("descricao = ?")
        values.append(nova_descricao)

    # Se não houver campos a atualizar, não faz nada
    if not update_fields:
        print("Erro: Nenhum dado para atualizar.")
        conn.close()
        return

    # Adiciona o 'tag' no final dos valores para a cláusula WHERE
    values.append(tag)

    # Monta a query de atualização
    query = f"UPDATE ativos SET {', '.join(update_fields)} WHERE tag = ?"
    
    cursor.execute(query, tuple(values))
    conn.commit()

    if cursor.rowcount > 0:
        print(f"Ativo {tag} atualizado com sucesso!")
    else:
        print(f"Erro ao tentar atualizar o ativo {tag}.")

    conn.close()

# Configuração do cliente MQTT para o Broker Privado
client = mqtt.Client()
client.on_message = on_message

# Configuração SSL/TLS
client.tls_set() 

# Conectar ao broker privado MQTT com autenticação
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD) 
client.connect(MQTT_BROKER, MQTT_PORT, 60)

# Subscribing aos tópicos
client.subscribe(MQTT_TOPIC_CADASTRAR)
client.subscribe(MQTT_TOPIC_ATUALIZAR)

# Iniciar o loop de escuta do MQTT
print("Aguardando mensagens MQTT...")
client.loop_forever()
