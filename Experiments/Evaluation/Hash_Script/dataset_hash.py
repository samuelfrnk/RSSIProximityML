# -*- coding: utf-8 -*-

import hashlib
def hash_file(file_path):
    with open(file_path, 'rb') as f:
        file_data = f.read()

    sha256 = hashlib.sha256(file_data).hexdigest()
    return sha256

file_path = 'combined_data.csv'
hash_value = hash_file(file_path)
print("SHA-256 Hash:", hash_value)
