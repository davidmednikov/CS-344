# #####################################################
# Program Py - Python Exploration
# David Mednikov
# CS344 Spring 2019
# #####################################################

# import random and string libraries
import random
import string

# global variable to store all lowercase letters
lowercase_letters = string.ascii_lowercase

# get_chars Function
# ----------------------------------------------------
# utility function to get 10-character string of random lowercase letters
# args: none
# output: 10-character string of random lowercase letters
# ----------------------------------------------------
def get_chars():
    # initialize empty string
    chars_string = ""

    # loop 10 times
    for _ in range(10):
        # get random letter from global list of lowercase letters
        letter = random.choice(lowercase_letters)

        # concatenate random letter to string
        chars_string += letter

    # return 10-character string
    return chars_string

# loop 3 times
for iterator in range(3):
    # get file number from iterator
    file_num = iterator + 1

    # set file_name based on file number
    file_name = "file_" + str(file_num)

    # create new file for writing with given file name
    file = open(file_name, "w+")

    # get 10-character string
    string = get_chars()

    # write 10-character string to file with newline, then close it
    file.write(string + "\n")
    file.close()

    # print 10-character tring to stdout
    print(string)

# generate random numbers and print them
num_1 = random.randint(1, 42)
num_2 = random.randint(1, 42)
print(num_1)
print(num_2)

# get product of random numbers and print it
product = num_1 * num_2
print(product)
