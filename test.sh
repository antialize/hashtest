for map in std ska flat crn tpie ordered; do
    echo "================================> large $map <======================================="
    ./hash linear_map $map perm large 50000000
done

for map in std ska flat crn tpie ordered; do
    echo "================================> small $map <======================================="
    ./hash linear_map $map perm small 400000000
done
