# cedlisp

Build the REPL and the interpreter (Note: required GNU readline and GNU make) :

`$ make release`

Launch the REPL:

`$ ./repl`

Load the standard library:

`[λ] ==> :library.ced`

Try out some expressions:

```
[λ] ==> (cat (range 5) (rev (range 5)))
(1 2 3 4 5 5 4 3 2 1)
[λ] ==> (len (range 10))
10
[λ] ==> (map println (range 3))
1
2
3
(() () ())
[λ] ==> (eval 'x)
12
[λ] ==> (eval (eval ''x))
12
[λ] ==> 'foo
foo
```

Ctrl + C to quit:
```
[λ] ==>
Goodbye!
```


For more, see the examples in the `examples` directory.
