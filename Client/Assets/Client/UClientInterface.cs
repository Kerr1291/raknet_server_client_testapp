using UnityEngine;
using System;
using System.Runtime;
using System.Collections;
using System.Runtime.InteropServices;
using ProtoBuf;

public class UClientInterface : MonoBehaviour {


    [DllImport("rlib")]
    public static extern IntPtr RCLIENT_GetErrorDesc();

    public string GetClientErrorDesc()
    {
        IntPtr desc = UClientInterface.RCLIENT_GetErrorDesc();
        string desc_str = Marshal.PtrToStringAnsi(desc);
        return desc_str;
    }

    [DllImport("rlib")]
    public static extern int RCLIENT_GetErrorState();

    public int GetClientErrorState()
    {
        return UClientInterface.RCLIENT_GetErrorState();
    }

    //Never call allocate more than once per client
    //There is no way to deallocate them (and no reason to, really)
    [DllImport("rlib")]
    public static extern int RCLIENT_AllocateNewClient();

    [DllImport("rlib")]
    public static extern void RCLIENT_ConnectToServer(int i0, int i1, int i2, int i3, int port, int client_port, int client_id);

    [DllImport("rlib")]
    public static extern int RCLIENT_GetConnectedState(int client_id);

    public bool IsConnectedToServer()
    {
        return (RCLIENT_GetConnectedState(_internal_client_id) > 0);
    }

    [DllImport("rlib")]
    public static extern void RCLIENT_DisconnectClient(int client_id);

    void OnApplicationQuit()
    {
        if (RCLIENT_GetConnectedState(_internal_client_id) >= 0)
            RCLIENT_DisconnectClient(_internal_client_id);
    }

    [DllImport("rlib")]
    public static extern int RCLIENT_GetPacket(ref IntPtr raw_message_id, ref IntPtr game_message_id, ref IntPtr packet_data, ref IntPtr bytes, int client_id);

    [DllImport("rlib")]
    public static extern int RCLIENT_FreeLastPacket(int client_id);

    [DllImport("rlib")]
    public static extern int RCLIENT_SendGamePacket(int game_message_id, byte[] packet_data, int bytes, int client_id);

    [DllImport("rlib")]
    public static extern int RCLIENT_SendRakNetPacket(int raknet_message_id, byte[] packet_data, int bytes, int client_id);





    //Unity Interface
    public string MyName = "Player";
    public int ServerIP_0 = 127;
    public int ServerIP_1 = 0;
    public int ServerIP_2 = 0;
    public int ServerIP_3 = 1;
    public int MyPort = 25506;
    public int ServerPort = 25505;

    //Private Members
    [SerializeField]
    int _internal_client_id = -1;


    public bool HasAllocatedClient()
    {
        if (_internal_client_id < 0)
            return false;
        return true;
    }

    public void CreateRakNetClient()
    {
        if(_internal_client_id < 0)
        {
            _internal_client_id = RCLIENT_AllocateNewClient();
        }
    }
    
    void Awake()
    {
    }

    public void ConnectToServer()
    {
        ConnectToServer(ServerIP_0, ServerIP_1, ServerIP_2, ServerIP_3, ServerPort);
    }

	void Update () {
        
    }

    //A cleaner way to get data during the game
    public int GetGamePacketData(ref int message_id, ref byte[] data, ref int bytes)
    {
        if (RCLIENT_GetConnectedState(_internal_client_id) == 0)
            return 0;

        IntPtr raw_message_id = new IntPtr(0);
        IntPtr game_message_id = new IntPtr(0);
        IntPtr byte_data = new IntPtr(0);
        IntPtr byte_count = new IntPtr(0);
        int result = RCLIENT_GetPacket(ref raw_message_id, ref game_message_id, ref byte_data, ref byte_count, _internal_client_id);


        if( result > 0 && game_message_id.ToInt32() >= 0 )
        {
            bytes = byte_count.ToInt32();
            data = new byte[bytes];
            Marshal.Copy(byte_data, data, 0, bytes);
            message_id = game_message_id.ToInt32();
        }
        else
        {
            RCLIENT_FreeLastPacket(_internal_client_id);
            return 0;
        }

        RCLIENT_FreeLastPacket(_internal_client_id);
        return 1;
    }

    void ProcessGamePackets()
    {
        if(!IsConnectedToServer())
            return;

        IntPtr raw_message_id = new IntPtr(0);
        IntPtr game_message_id = new IntPtr(0);
        IntPtr byte_data = new IntPtr(0);
        IntPtr bytes = new IntPtr(0);
        int result = RCLIENT_GetPacket(ref raw_message_id, ref game_message_id, ref byte_data, ref bytes, _internal_client_id);


        while(result > 0)
        {
            int byte_count = bytes.ToInt32();
            byte[] message_data = new byte[byte_count];
            Marshal.Copy(byte_data, message_data, 0, byte_count);

            if (game_message_id.ToInt32() >= 0)
              ProcessGameMessage(game_message_id.ToInt32(), message_data, bytes.ToInt32());

            result = RCLIENT_GetPacket(ref raw_message_id, ref game_message_id, ref byte_data, ref bytes, _internal_client_id);
        }

        RCLIENT_FreeLastPacket(_internal_client_id);
    }
    

    void ProcessGameMessage(int message_id, byte[] data, int bytes)
    {
        int value = Convert.ToInt32(rproto.RMessageType.msg_srequest);
        int value2 = Convert.ToInt32(rproto.RMessageType.msg_chat);

        if (message_id == value )
        {
            System.IO.Stream stream = new System.IO.MemoryStream(data, 0, bytes);
            rproto.SRequest request = ProtoBuf.Serializer.Deserialize<rproto.SRequest>(stream);

            if (request.request_id == rproto.SRequest.RequestType.r_pinfo)
            {
                rproto.PlayerInfo pinfo = new rproto.PlayerInfo();
                pinfo.name = MyName;
                pinfo.ip = "127.0.0.1";

                byte[] msg_data = new byte[1024];
                System.IO.Stream pinfo_stream = new System.IO.MemoryStream(msg_data);
                ProtoBuf.Serializer.Serialize<rproto.PlayerInfo>(pinfo_stream, pinfo);

                int message_id_to_send = Convert.ToInt32(rproto.RMessageType.msg_player_info);
                int bytes_to_send = Convert.ToInt32(pinfo_stream.Position);
                Debug.Log("sending " + bytes_to_send + " bytes in a pinfo message");

                RCLIENT_SendGamePacket(message_id_to_send, msg_data, bytes_to_send, _internal_client_id);
            }
        }
        else
        if (message_id == value2)
        {
            string text;
            string owner;

            System.IO.Stream stream = new System.IO.MemoryStream(data, 0, bytes);
            rproto.ChatMessage chatmsg = ProtoBuf.Serializer.Deserialize<rproto.ChatMessage>(stream);

            text = chatmsg.content;
            owner = chatmsg.owner_name;
            Debug.Log( "Test Client:: " + owner + " says: " + text );
        }
    }
    
    void ConnectToServer(int ip0, int ip1, int ip2, int ip3, int port)
    {
        if (IsConnectedToServer())
            return;

        ServerIP_0 = ip0;
        ServerIP_1 = ip1;
        ServerIP_2 = ip2;
        ServerIP_3 = ip3;

        ServerPort = port;

        RCLIENT_ConnectToServer(ServerIP_0, ServerIP_1, ServerIP_2, ServerIP_3, ServerPort, MyPort, _internal_client_id);
        Debug.Log("Trying to connect to " + ip0 + "." + ip1 + "." + ip2 + "." + ip3 + ":" + port);
    }

    public void SendMessageToServer(string msg)
    {
        if (!IsConnectedToServer())
            return;

        rproto.ChatMessage chatty = new rproto.ChatMessage();
        chatty.owner_name = MyName;
        chatty.owner_ip = UnityEngine.Network.player.ipAddress; //not really used... need to get rid of this
        chatty.msg_type = rproto.ChatMessage.ChatType.chat_normal;
        chatty.content = msg;

        byte[] msg_data = new byte[1024];
        System.IO.Stream msg_stream = new System.IO.MemoryStream(msg_data);
        ProtoBuf.Serializer.Serialize<rproto.ChatMessage>(msg_stream, chatty);
        

        int message_id_to_send = Convert.ToInt32(rproto.RMessageType.msg_chat);
        int bytes_to_send = Convert.ToInt32(msg_stream.Position);
        //Debug.Log("sending " + message_id_to_send + " id ");
        //Debug.Log("sending " + bytes_to_send + " bytes in a chat message");

        RCLIENT_SendGamePacket(message_id_to_send, msg_data, bytes_to_send, _internal_client_id);
    }

    public void SendPlayerInfoMessage()
    {
        if (!IsConnectedToServer())
            return;

        rproto.PlayerInfo pinfo = new rproto.PlayerInfo();
        pinfo.name = MyName;
        pinfo.ip = UnityEngine.Network.player.ipAddress;

        byte[] msg_data = new byte[1024];
        System.IO.Stream pinfo_stream = new System.IO.MemoryStream(msg_data);
        ProtoBuf.Serializer.Serialize<rproto.PlayerInfo>(pinfo_stream, pinfo);

        int message_id_to_send = Convert.ToInt32(rproto.RMessageType.msg_player_info);
        int bytes_to_send = Convert.ToInt32(pinfo_stream.Position);

        RCLIENT_SendGamePacket(message_id_to_send, msg_data, bytes_to_send, _internal_client_id);
    }
}
