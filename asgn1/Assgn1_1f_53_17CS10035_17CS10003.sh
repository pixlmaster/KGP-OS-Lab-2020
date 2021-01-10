#input_file="1f.graph.edgelist"
temp_file="1f.tmp"
temp_file_2="1f2.tmp"

rm -f $temp_file
rm -f $temp_file_2

input_file="1f.graph.edgelist"
tr -d "'\[\]" < $input_file > $temp_file
 
output_file="1f_output_graph.edgelist"

awk '{if ($1==$2){} else if ($1 <$2){print $1, $2} else {print $2,$1}}' < $temp_file > $temp_file_2
sort $temp_file_2 > $temp_file
uniq $temp_file > $output_file

tr " " "\n" < $output_file > $temp_file
sort -n $temp_file > $temp_file_2
uniq -c $temp_file_2 > $temp_file
sort -nr -k1 $temp_file > $temp_file_2

echo "Node Degree" 
head -5 $temp_file_2 |
while read line; do
	x=( $line )
	echo "${x[1]} ${x[0]}"
done

rm -f $temp_file
rm -f $temp_file_2

