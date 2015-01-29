#! /bin/sh

# 1B: Test that execution works

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >testb.sh <<'EOF'
echo hi

echo testing blah hi | tr a-z A-Z

echo testing blah hi | tr a-z A-Z | tr A 4

( echo testing subshell )

if true 
then echo it was tru 
fi

if false 
then echo oops 
else echo yay 
fi

echo hello ; : ; echo again
EOF

cat >testb.exp <<'EOF'
hi
TESTING BLAH HI
TESTING BL4H HI
testing subshell
it was tru
yay
hello
again
EOF

../profsh testb.sh >testb.out 2>testb.err || exit

diff -u testb.exp testb.out || exit
test ! -s testb.err || {
  cat testb.err
  exit 1
}

) || exit

rm -fr "$tmp"