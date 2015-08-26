

nc 127.0.0.1 12345 <in2 > out1 & 
nc 127.0.0.1 12345 <in2 > out2 &
nc 127.0.0.1 12345 <in2 > out3 & 
nc 127.0.0.1 12345 <in2 > out4 &
nc 127.0.0.1 12345 <in2 > out5 & 
nc 127.0.0.1 12345 <in2 > out6 


echo "Difference between the messages received by the 6  clients"
diff out1 out2
diff out2 out3
diff out3 out4
diff out4 out5
diff out5 out6



