using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SetClientVars : MonoBehaviour {

    public InputField input;

    public UClientInterface client;

    public void SetClientPort()
    {

        int port = System.Convert.ToInt32(input.text);

        client.MyPort = port;

    }

    public void SetServerPort()
    {

        int port = System.Convert.ToInt32(input.text);

        client.ServerPort = port;

    }

    public void SetClientName()
    {
        client.MyName = input.text;
    }

    public void SetClientIP()
    {
        System.Net.IPAddress ip = System.Net.IPAddress.Parse(input.text);
        byte[] ip_bytes = ip.GetAddressBytes();

        client.ServerIP_0 = ip_bytes[0];
        client.ServerIP_1 = ip_bytes[1];
        client.ServerIP_2 = ip_bytes[2];
        client.ServerIP_3 = ip_bytes[3];

        Debug.Log("ip address set to " + ip_bytes[0] + "." + ip_bytes[1] + "." + ip_bytes[2] + "." + ip_bytes[3]);
    }
}
