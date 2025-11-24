# monitoramento_placa_veicular

Projeto Integrado – Sistema de Leitura Automática de Placas Veiculares

Projeto desenvolvido na universidade UNIFEOB com o objetivo de aumentar a segurança e automatizar processos de portaria em empresas. Esta implementação foi criada para atender às necessidades da PackBag, empresa parceira da instituição e atuante no setor de Big Bags.

Descrição do Projeto

Este projeto tem como objetivo automatizar o processo de identificação de veículos em portarias, utilizando uma câmera ESP32-CAM para leitura de placas veiculares.
Ao capturar uma imagem, o sistema envia a foto para uma API capaz de reconhecer os caracteres da placa, registra os dados no Firebase e integra o monitoramento ao banco MySQL, permitindo histórico, auditoria e controle de acesso de forma automatizada.

Como o Sistema Funciona:

  Captura da Placa
    A ESP32-CAM é configurada via Arduino para capturar a imagem da placa do veículo.


  Envio da Imagem para a API
    A imagem capturada é enviada para a API da PlateRecognizer, que retorna:

    •	Texto da placa identificada

    •	Porcentagem de confiança/precisão da leitura

  Processamento no Arduino
    O Arduino recebe o retorno da API e envia para o Firebase: 

    •	ID da câmera

    •	Placa identificada

    •	Porcentagem de acerto

    •	Data e horário exato da captura

  Armazenamento no Firebase

    Todas as leituras são registradas no Firebase para consulta futura.

    Monitoramento e a Integração com MySQL

    No menu do projeto, a opção “7 – Acessar Monitoramento de Placas” recupera os dados do Firebase, exibe para o usuário e armazena as informações no banco MySQL, completando o ciclo de monitoramento.

  Tecnologias Utilizadas
    ESP32-CAM / Arduino IDE	  -  Captura e envio das imagens
    API PlateRecognizer		    -  Leitura e reconhecimento da placa
    Firebase			            -	 Armazenamento de dados e autenticação
    MySQL			                -	 Banco de dados final para o sistema
    Flutter / Dart			      -	 Desenvolvimento do aplicativo 
    Visual Studio Code		    -  Desenvolvimento e organização do projeto

  API Utilizada
    PlateRecognizer – Automatic License Plate Recognition (ALPR)
    https://platerecognizer.com/

  Como Utilizar o Sistema
    1.	Ligar a ESP32-CAM e posicioná-la para capturar a placa.
    2.	A imagem será enviada automaticamente para a API.
    3.	A leitura será armazenada no Firebase.
    4.	No Dart, acessar a opção: 7 – Monitoramento de Placas
    5.	O app exibirá todas as leituras e registrará no MySQL quando necessário.
