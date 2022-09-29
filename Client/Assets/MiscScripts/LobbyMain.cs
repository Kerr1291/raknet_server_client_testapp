using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using ProtoBuf;

public class LobbyMain : MonoBehaviour {

    public UClientInterface me;
    public CServerInterface me_server;
    public GameObject lobby_ui_root;
    public UnityEngine.UI.InputField chat_input;
    public UnityEngine.UI.Text chat_output;
    
    bool server_starting_connect = false;
    bool server_waiting_to_connect = false;
    float connect_delay_check = 0;
    bool connected = false;

    bool client_starting_connect = false;
    bool client_waiting_to_connect = false;

    bool in_lobby = false;

    void CheckClientConnection()
    {
        if (connect_delay_check <= 0)
        {
            connect_delay_check = 1.0f;

            if (me.IsConnectedToServer())
            {
                connected = true;
                Debug.Log("Client successfuly connected.");
            }
        }
        else
        {
            connect_delay_check -= Time.deltaTime;
        }
    }

    void CheckConnection()
    {
        if (server_starting_connect)
        {
            if (!server_waiting_to_connect && CServerInterface.GetServerReady())
            {
                if (!me.HasAllocatedClient())
                    me.CreateRakNetClient();

                me.ServerPort = CServerInterface.GetServerPort();
                me.MyPort = me.ServerPort + 1;
                me.ServerIP_0 = 127;
                me.ServerIP_1 = 0;
                me.ServerIP_2 = 0;
                me.ServerIP_3 = 1;
                me.ConnectToServer();
                server_waiting_to_connect = true;
            }
            else if (server_waiting_to_connect)
            {
                if( CServerInterface.HasServerError() )
                {
                    Debug.LogError(CServerInterface.GetServerError());
                    server_waiting_to_connect = false;
                    server_starting_connect = false;
                    me_server.KillServer();
                    return;
                }

                if (me.GetClientErrorState() > 0)
                {
                    Debug.LogError(me.GetClientErrorDesc());
                    server_waiting_to_connect = false;
                    server_starting_connect = false;
                    me_server.KillServer();
                    return;
                }


                CheckClientConnection();
            }
        }

        if (client_starting_connect)
        {
            if (!client_waiting_to_connect)
            {
                me.ConnectToServer();
                client_waiting_to_connect = true;
            }
            else
            {
                if (me.GetClientErrorState() > 0)
                {
                    Debug.LogError(me.GetClientErrorDesc());
                    client_starting_connect = false;
                    client_waiting_to_connect = false;
                    return;
                }

                CheckClientConnection();
            }
        }
    }

    void LoadLobby()
    {
        lobby_ui_root.SetActiveRecursively(true);

        //TODO: request players from server

        in_lobby = true;
    }
    
    void Update () {

        if (!connected)
            CheckConnection();
        else
        {
            if (!in_lobby)
                LoadLobby();

            ProcessLobbyPackets();
        }
    }

    void ProcessLobbyPackets()
    {
        if (!in_lobby)
            return;

        int message_id = 0;
        byte[] data = null;
        int bytes = 0;
        int result = me.GetGamePacketData(ref message_id, ref data, ref bytes);

        //in this case it's a game packet, process it
        while( result > 0 )
        {
            //send a player info message in reponse to a server request
            if(message_id == Convert.ToInt32(rproto.RMessageType.msg_srequest))
            {
                System.IO.Stream stream = new System.IO.MemoryStream(data, 0, bytes);
                rproto.SRequest request = ProtoBuf.Serializer.Deserialize<rproto.SRequest>(stream);

                if(request.request_id == rproto.SRequest.RequestType.r_pinfo)
                {
                    me.SendPlayerInfoMessage();
                }
            }


            //respond to a chat message in the lobby
            if (message_id == Convert.ToInt32(rproto.RMessageType.msg_chat))
            {
                string text;
                string owner;

                System.IO.Stream stream = new System.IO.MemoryStream(data, 0, bytes);
                rproto.ChatMessage chatmsg = ProtoBuf.Serializer.Deserialize<rproto.ChatMessage>(stream);

                text = chatmsg.content;
                owner = chatmsg.owner_name;

                PostChatToLog(owner, text);
            }

            result = me.GetGamePacketData(ref message_id, ref data, ref bytes);
        }
    }

    public void StartServerAndConnect()
    {
        me_server.StartGameServer();

        server_starting_connect = true;
    }

    public void ConnectAsClientOnly()
    {
        client_starting_connect = true;
    }

    public void SendLobbyChat()
    {
        string text = chat_input.text;

        me.SendMessageToServer(text);

        chat_input.text = "";
    }

    public void PostChatToLog(string owner, string msg)
    {
        string formatted_chat = owner + " says: " + msg + "\r\n";

        chat_output.text += formatted_chat;
    }
}
