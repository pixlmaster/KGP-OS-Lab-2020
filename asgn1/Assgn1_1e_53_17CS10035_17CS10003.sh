file=$1
temp_file="1e.tmp"
temp_file_2="1e2.tmp"
output_file="1e_output_"$2"_column.freq"

awk '{ print tolower($'$2') }' "$file" > $temp_file
cat $temp_file

sort -o $temp_file_2 $temp_file
uniq -i -c $temp_file_2 > $temp_file
sort -nr -k1 $temp_file > $temp_file_2
awk '{ print $2" "$1 }' $temp_file_2 > $output_file
rm -f $temp_file_2
rm -f $temp_file
