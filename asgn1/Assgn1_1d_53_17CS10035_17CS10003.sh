folder="1.d.files"
folder_out="1.d.files.out"

rm -r -f "$folder_out"
mkdir "$folder_out"

for i in "$folder"/*
do
	sort -nr -o "$folder_out/${i##*/}" "$i"
done


sort -nr -o "1.d.out.txt" "$folder_out"/*