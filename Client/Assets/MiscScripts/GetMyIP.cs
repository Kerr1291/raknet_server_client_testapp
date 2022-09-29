using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class GetMyIP : MonoBehaviour {
    
    public Text text;

    void Awake()
    {
      text.text = UnityEngine.Network.player.ipAddress;
    }
}
