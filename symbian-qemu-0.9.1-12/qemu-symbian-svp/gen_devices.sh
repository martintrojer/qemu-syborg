#! /bin/sh
# Call device init functions.

file="$1"
shift
devices="$@"
echo '/* Generated by gen_devices.sh */' > $file
for x in $devices ; do
    echo "void ${x}_register(void);" >> $file
done
echo "void register_devices(void)" >> $file
echo "{" >> $file
for x in $devices ; do
    echo "    ${x}_register();" >> $file
done
echo "}" >> $file
