ex:
```
gcc -Wall -Werror -g main.c && \
    lldb -o run ./a.out "((λf.λx.(f)((f)((f)(x))))(+1))(0)"
```

also has tiny repl if you don't provide a command line arg
```
./a.out
input:
((λf.λx.(f)(x))(+1))(0)
parsed 9 nodes: ((λf.λx.(f)(x))(+1))(0)
eval result: 1
```
