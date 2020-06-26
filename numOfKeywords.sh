#!/bin/bash

declare -a resArray=()
index=0

for file in $PWD/log/*
do
	find "$file" | grep ".txt"
	while read -r line || [[ -n "$line" ]]
	do
		name="$line"	#name has the whole line
		qtype=${name:21:6}	#qtype has the query type--we care for search queries
		if [ "$qtype" = "search" ]
		then
			temp=${name:29}
			query=${temp%%:*}
			query=${query##*/}	#query has the query!
			if [[ ${#resArray[@]} -eq 0 ]]
			then
				resArray[$index]="$query"
				let index++
			fi
			counter=0
			while [[ $counter -lt $index ]]
			do
				if [ "$query" == "${resArray[$counter]}" ]
				then
					break
				fi
				let counter++
			done
			if [[ $counter -eq $index ]]
			then
				resArray[$index]="$query"
				let index++
			fi
		fi
	done < "$file"
done

echo "Total number of keywords searched"
echo $index