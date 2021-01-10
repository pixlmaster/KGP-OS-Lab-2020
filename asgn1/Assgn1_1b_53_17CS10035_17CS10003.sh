rm -f "1b_output.txt"
input_file=1b_input.txt
i=1
while read line; do
	echo "$i $line" >> "1b_output.txt" 
	((i++))
done < $input_file
#rm -f output.txt
#mv output.txt 1b_input.txt
