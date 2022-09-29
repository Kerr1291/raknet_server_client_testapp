using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;

public class CServerInterface : MonoBehaviour {

    [DllImport("rlib")]
    public static extern int RSERVER_GetNumParams();

    [DllImport("rlib")]
    public static extern IntPtr RSERVER_GetParamDesc(int param_id);

    [DllImport("rlib")]
    public static extern void RSERVER_ProcessOutParameter(int param_id, out IntPtr data);

    [DllImport("rlib")]
    public static extern void RSERVER_ProcessParameter(int param_id, ref IntPtr data);

    [DllImport("rlib")]
    public static extern int  RSERVER_GetServerGamePort();

    IntPtr _server_nullptr;
    IntPtr _server_int_result;
    IntPtr _server_string_result;


    //Unity interface
    public int ServerPort = 25505;

    public bool ServerRunning = false;

    public bool DebugStartServer = false;

    void OnApplicationQuit()
    {
        KillServer();
    }

    public void KillServer()
    {
        if (ServerRunning)
        {
            IntPtr inull = new IntPtr(0);
            RSERVER_ProcessParameter(0, ref inull);
            ServerRunning = false;
        }
    }

    void Awake()
    {
    }

    public static void StartServer()
    {
        IntPtr server_nullptr = new IntPtr();
        RSERVER_ProcessParameter(1, ref server_nullptr);
    }

    public static void SetServerPort(int port)
    {
        Debug.Log("Setting server port to " + port);
        IntPtr iport = new IntPtr(port);
        RSERVER_ProcessParameter(4, ref iport);
    }

    public static int GetServerParamMax()
    {
        return RSERVER_GetNumParams();
    }

    public static string GetServerParamDesc(int param_id)
    {
        IntPtr desc = RSERVER_GetParamDesc(param_id);
        string desc_str = Marshal.PtrToStringAnsi(desc);
        return desc_str;
    }

    public static int GetServerPort()
    {
        return RSERVER_GetServerGamePort();
    }

    public static bool GetServerReady()
    {
        IntPtr server_int_result = new IntPtr(0);
        RSERVER_ProcessParameter(5, ref server_int_result);
        Debug.Log("Server Is Ready = " + (server_int_result == new IntPtr(1)));
        return (server_int_result == new IntPtr(1));
    }

    public static bool HasServerError()
    {
        IntPtr server_int_result = new IntPtr(0);
        RSERVER_ProcessParameter(6, ref server_int_result);
        //Debug.Log("Server Has Error = " + (server_int_result == new IntPtr(1)));
        return (server_int_result == new IntPtr(1));
    }

    public static string GetServerError()
    {
        IntPtr server_string_result;
        RSERVER_ProcessOutParameter(7, out server_string_result);
        string server_error_str = Marshal.PtrToStringAnsi(server_string_result);

        Debug.Log("Server Error = " + server_error_str);
        return server_error_str;
    }

    /// <summary>
    /// Normal functions start here
    /// </summary>

    public void StartGameServer()
    {
        CServerInterface.SetServerPort(ServerPort);
        CServerInterface.StartServer();
        ServerRunning = CServerInterface.GetServerReady();
    }

    
	
	void Update () {
        
    }
}
