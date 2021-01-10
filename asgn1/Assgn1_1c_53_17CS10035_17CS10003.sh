find_gcd()
{
	ans=0
	zero_comp=0
	if [ $1 -eq $2 ]
	then
		ans=$1
	elif [ $1 -gt $2 ]
	then
		a=$1
		b=$2
		while [ $b -ne $zero_comp ]
		do
			c=$b
			b=`expr $a % $b`
			a=$c

		done
		ans=$a
	else

		a=$2
		b=$1
		while [ $b -ne $zero_comp ]
		do
			c=$b
			b=`expr $a % $b`
			a=$c

		done
		ans=$a
	fi

	echo $ans
}


count=1
gcd=0
for i in $(echo $1 | sed "s/,/ /g")
do
	if [ $count -eq 1 ]
	then
		gcd=$i
	else
		gcd=$(find_gcd $gcd $i)
    fi

    count=`expr $count + 1`
done

echo $gcd
