def rev_str(s):
    return s[:: - 1]
s = '/bin/id'
s= rev_str(s)
for i in s:
    t=hex(ord(i))
    print(t[2:])

