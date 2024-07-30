import numpy as np

def generate_linear_dataset():
    """
    Generate a linear dataset and write it to the file 'linear'.
    The dataset contains integers from 1 to num_elements-1.
    """
    num_elements = 200000000
    with open('linear', 'wb') as f:
        # Write the number of elements
        f.write(num_elements.to_bytes(8, 'little'))
        for i in range(1, num_elements):
            # Write each integer
            f.write(i.to_bytes(8, 'little'))

def generate_fb_dataset():
    """
    Read data from the file 'fb', subtract 1 from each element,
    and write the result to the file 'fb-1'.
    """
    fb = np.fromfile('fb', dtype=np.uint64)[1:]
    fb_1 = fb - 1
    num_elements = len(fb_1)
    with open('fb-1', 'wb') as f:
        # Write the number of elements
        f.write(num_elements.to_bytes(8, 'little'))
        for i in fb_1:
            i = int(i)
            # Write each integer
            f.write(i.to_bytes(8, 'little'))

if __name__ == '__main__':
    generate_linear_dataset()
    generate_fb_dataset()