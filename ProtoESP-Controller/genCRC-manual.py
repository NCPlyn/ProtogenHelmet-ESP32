import binascii

with open('data\config.json', 'r') as file:
  print("Generating CRC...")
  data = file.read().replace('\n', '').replace(' ', '')
  crc = binascii.crc32(data.encode())
  print(crc)
  with open('data\configCRC.txt', 'w') as file2:
    file2.write(str(crc))
    file2.close()
  file.close()
print("Generated CRC")