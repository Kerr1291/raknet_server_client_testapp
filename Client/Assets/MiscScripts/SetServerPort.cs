using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SetServerPort : MonoBehaviour {

    public InputField input;

    public void SetServerGamePort()
    {
        int port = System.Convert.ToInt32( input.text );

        CServerInterface.SetServerPort(port);
    }



}
