
#if 0
/*
 * WIZnet mosquitto server
 * Host name: mqtt.wiznet.io
 * Port: 1883(No SSL), 8883(SSL)
 *
 */
const char	root_ca[] =	\
"-----BEGIN CERTIFICATE-----\r\n"   \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"   \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"   \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"   \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"   \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"   \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"   \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"   \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"   \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"   \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"   \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"   \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"   \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"   \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"   \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"   \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"   \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"   \
"rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"   \
"-----END CERTIFICATE-----\r\n";

const char client_cert[] = \
"-----BEGIN CERTIFICATE-----\r\n"   \
"MIIDWTCCAkGgAwIBAgIUM33b9vSZyXxgtX1rZKJXPQnYL00wDQYJKoZIhvcNAQEL\r\n"   \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\r\n"   \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMDIwMTIzMzgx\r\n"   \
"MloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\r\n"   \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALhuDE0y7cB3jh1mmeQo\r\n"   \
"4CViHQKDWJ7+ZxXRdHfPNX825ZnUn2lX8ABJjyQihCmq3FJqfQ2BDrhINVlWOvq0\r\n"   \
"MVn0vF7Nk8T4dTfF4Qq9T6PoU4C9l6C3xKfzkkkGROwvQyWxZGB8k6dqfkCsp2ZF\r\n"   \
"uEVE4DYCrBSCzJj2pISB4rN0vpc5tU4CwB3GxrqXujZ77yT1bNwajrgL1k8ZSQQ3\r\n"   \
"mOLDTz57cr6vZA+Kkn8eSerq0lk3lUIYByI0k4DoYRh+9ANqAkav6bBznmplo9eI\r\n"   \
"t89U2Q48JVoPOoWYNCbgTu/UhKnl5JC9ZPxpx6l6yOcgUKOynoACfsPd2bd85w28\r\n"   \
"4O8CAwEAAaNgMF4wHwYDVR0jBBgwFoAUA3DYFyk/jnD5yrCpisbApUp/ohwwHQYD\r\n"   \
"VR0OBBYEFGS9biJCQd4mjLxhoRkK4wmMmpAjMAwGA1UdEwEB/wQCMAAwDgYDVR0P\r\n"   \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBgC+H+6/Uai/NUIb/6joMHC9X3\r\n"   \
"gTRd3SSW0BLlSuoj94B30eHGUDGe0X4i5TaylQNZjenYz/yhyFJMoNautOOElFjA\r\n"   \
"ihk60P+xv8RkOtukuAAaTDrx4NKWOVbY9siZc5ppgB1lpHZQ0DlcF+sAm0g55Dn/\r\n"   \
"Ya9F/wgjMGq4B7U6+3S/7wBCDtxAUcWsCtXZBoKLxctH4ZrNggrocRWWqCrBW59H\r\n"   \
"CbM2Sj3obqVZg3hArRvkenF5/uPh9Kp2VQMT0LNlQwti5v2MqYGRYrfhKgNoFKSC\r\n"   \
"cV6DJkzMkAxjN7QPXsyPcnCiUXgixHpukY7njsA1c0cuBiSdsgRsSDZbzS3Y\r\n"   \
"-----END CERTIFICATE-----\r\n";

const char private_key[] = \
"-----BEGIN RSA PRIVATE KEY-----\r\n"   \
"MIIEogIBAAKCAQEAuG4MTTLtwHeOHWaZ5CjgJWIdAoNYnv5nFdF0d881fzblmdSf\r\n"   \
"aVfwAEmPJCKEKarcUmp9DYEOuEg1WVY6+rQxWfS8Xs2TxPh1N8XhCr1Po+hTgL2X\r\n"   \
"oLfEp/OSSQZE7C9DJbFkYHyTp2p+QKynZkW4RUTgNgKsFILMmPakhIHis3S+lzm1\r\n"   \
"TgLAHcbGupe6NnvvJPVs3BqOuAvWTxlJBDeY4sNPPntyvq9kD4qSfx5J6urSWTeV\r\n"   \
"QhgHIjSTgOhhGH70A2oCRq/psHOeamWj14i3z1TZDjwlWg86hZg0JuBO79SEqeXk\r\n"   \
"kL1k/GnHqXrI5yBQo7KegAJ+w93Zt3znDbzg7wIDAQABAoIBAHoFVV4+M88DSqNp\r\n"   \
"Hqg14xbYsxjWjeujpdBppAUgsuRYDdCZPF8WL1QBVxMxQv/Oa7abfeFRYcvK+oEA\r\n"   \
"7ESys3Qw1/BhU0Men28nHJ1eYzKuo5Cp34gWDaaRIFfwLemV23c0xpSP16EMdDuV\r\n"   \
"/6tKtYAe8bjKS8/GOld5159T0AmqMqln1pr0TQ+jNauQ/5IOUPTK19gPd7FAU8CO\r\n"   \
"I0+mOnVBujepmsQFFBdvwAQoZMTKqY1xE5aILvO88bei8zPGtGl1tckeGd00chzR\r\n"   \
"Y+9Q/E0+YXM4di/KIiiW5vxXcFmEBfZgm6kD9MWFG8tyYjko4WbmUysR0rAAig0w\r\n"   \
"TZX5AQECgYEA8ppV7vDguPpTTPl10em46ki61lCZh7eaeqzldTsi0bSuHcOeQzRC\r\n"   \
"XclI5Hp/eYO4R4ZvOy1537g49L9vVv++kuc3+oxddLDE7WGx2yGkyVuAF7H7DCzy\r\n"   \
"3piIa9kQb6PFD5J6JDMM8m22CSDN3/wS8aYKSWowd3mIbCU8AfXSxr0CgYEAwp1S\r\n"   \
"3ZXvuadqwOTNAxwf9GevFUy8cn+jxc6W/h5r0Kc1mQ1zdxCRKB0MEvpeVhZu6keS\r\n"   \
"sJDFCogkNTYcBZ37p6yqwsZ8MWO7EgrChB/yCFioFMNyKiGVi829gYfSoZsaCmEQ\r\n"   \
"lKqWAqetUBng+UlznT6ak1RTpcqxSHXCJ17pxxsCgYBqbQYxsf4DZ91PMUhcc9mo\r\n"   \
"/YrpJaaUgRkFo/9tNCoMMOUJLZ2qAazzk6+ke5CccApRZko5fbOvTxO9WpWvZrXl\r\n"   \
"oAVQpm8BF+Lr+CWb6eBubPN+cZeeOcG5oNSH60tojkGOmUV3x+VwHGz1CC0Ii36L\r\n"   \
"/7kXh7wHTYtpVdRDT+HNSQKBgFgyvmxH2EX2Nq5Gwxamt2CnYLSxezIU4J7z2oA+\r\n"   \
"PzRnoyXC79JFAtQAHgm3x+sR7aBlYXh9k58LRgWLiTWOfI9n9FejZoNnpE8v6ym/\r\n"   \
"5IudykbmsKWhZMngKwM6D95HkYykosQNb0CyxxvnLosrm6bmyVz9uY8IhkHFhIEd\r\n"   \
"RauDAoGAY8BjeRw4qdTioVOj03zoK4VULuDZmBo8286ANMir6TWoC39TIUEMmDGq\r\n"   \
"snACOA/Cv9gmayDaVKdatk+HxhQ4aBPAI6mqUtYcIwIx4YfxPzc+R+DRSPXCOdL/\r\n"   \
"G/OIwu2VAD2xLBd/XPrlkLsBedsoeMWxo2hmmF+RRxbJ0M6j1mU=\r\n"   \
"-----END RSA PRIVATE KEY-----\r\n";
#endif


