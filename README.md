# minimum-pattern-cutting

Requirement: 
if tag == "\t" :
    split(tag)
else if tag == " " :
    subject = substr(first space)
    predicate = substr(first space + 1, second space)
    object = substr(second space + 1)

Tips:
Analyze memery use according to 10M 
ofstream out\[partCnt\] 