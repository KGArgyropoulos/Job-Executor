#!/bin/bash

declare -a resArray=()
declare -a scArray=()
declare -a pathArray=()
declare -a isSearched=()
index=0

for file in $PWD/log/*
do
	find "$file" | grep ".txt"
	while read -r line || [[ -n "$line" ]]
	do
		name="$line"	#name has the whole line
		qwctype=${name:21:2}
		if [ "$qwctype" = "wc" ]
		then
			break
		fi
		qtype=${name:21:6}	#qtype has the query type
		###############################################
		if [ "$qtype" = "search" ]
		then
			temp=${name:29}
			query=${temp%%:*}
			query=${query##*/}	#query has the query!
			path=${temp#*: }	#path has the path!
			res="${temp//[^:]}"
			numOfPaths=${#res}
			if [[ ${#resArray[@]} -eq 0 ]]
			then
				resArray[$index]="$query"
				scArray[$index]=$numOfPaths
				pathArray[$index]="$path"
				isSearched[$index]=1
				let index++
			fi
			counter=0
			while [[ $counter -lt $index ]]
			do
				if [ "$query" == "${resArray[$counter]}" ]
				then
					if [ "$path" == "${pathArray[$counter]}" ]
					then
						break
					else
						temp=0
						while [[ temp -lt numOfPaths ]]
						do
							let scArray[$counter]++
							let temp++
						done
						pathArray[$counter]="$path"
						break
					fi
				fi
				let counter++
			done
			if [[ $counter -eq $index ]]
			then
				resArray[$index]="$query"
				scArray[$index]=$numOfPaths
				pathArray[$index]="$path"
				isSearched[$index]=1
				let index++
			fi			
		###############################################
		else	#min/maxcount results
			temp=${name:31}
			query=${temp%%:*}
			query=${query##*/}	#query has the query!
			path=${temp#*: }	#path has the path!
			if [[ ${#resArray[@]} -eq 0 ]]
			then
				resArray[$index]="$query"
				scArray[$index]=1
				pathArray[$index]="$path"
				isSearched[$index]=0
				let index++
			fi
			counter=0
			while [[ $counter -lt $index ]]
			do
				if [ "$query" == "${resArray[$counter]}" ]
				then
					if [ "$path" == "${pathArray[$counter]}" ]
					then
						break
					fi
					if [[ 0 -eq ${isSearched[$counter]} ]]
					then
						let scArray[$counter]++
						pathArray[$counter]="$path"
						break
					fi
					break
				fi
				let counter++
			done
			if [[ $counter -eq $index ]]
			then
				resArray[$index]="$query"
				scArray[$index]=1
				pathArray[$index]="$path"
				isSearched[$index]=0
				let index++
			fi
		fi
	done < "$file"
done
#find max and min of the array
readarray -t sorted < <(for a in "${scArray[@]}"; do echo "$a"; done | sort)
min=${sorted[0]}
max=${sorted[index-1]}
#print the results
echo "keywords most frequently found:"
i=0
while [[ $i -lt index ]]
do
	if [[ $max -eq ${scArray[$i]} ]]
	then
		echo -n ${resArray[$i]}
		echo -n "["
		echo -n ${scArray[$i]}
		echo "]"
	fi
	let i++
done
#print results
echo "keywords least frequently found:"
i=0
while [[ $i -lt index ]]
do
	if [[ $min -eq ${scArray[$i]} ]]
	then
		echo -n ${resArray[$i]}
		echo -n "["
		echo -n ${scArray[$i]}
		echo "]"
	fi
	let i++
done