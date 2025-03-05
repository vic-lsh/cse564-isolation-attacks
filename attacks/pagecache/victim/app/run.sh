#!/bin/bash
/usr/bin/time -f %e head -c $(cat bytes_to_read) random20.img > /dev/null
/usr/bin/time -f %e head -c $(cat bytes_to_read) random20.img > /dev/null
rm bytes_to_read # consume it
