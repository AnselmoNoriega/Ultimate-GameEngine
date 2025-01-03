import os

# Function to get all available drives on the system
def get_all_drives():
    # Get the available drives on the system
    drives = []
    for drive in range(65, 91):  # ASCII codes for A-Z (65 to 90)
        drive_letter = f"{chr(drive)}:"
        if os.path.exists(drive_letter):
            drives.append(drive_letter)
    return drives

# Function to search for the libfbxsdk-md.lib file
def find_fbx_sdk():
    # Get all drives on the system
    drives = get_all_drives()
    
    # Define the file to look for
    file_to_find = "libfbxsdk-md.lib"
    
    # Walk through directories on each drive and search for the file
    for drive in drives:
        for root, dirs, files in os.walk(drive):
            if file_to_find in files:
                return root  # Return the directory where the file is located
    print(f"{file_to_find} not found on any drive.")
    return None

# Function to set the environment variable
def set_fbx_sdk_env():
    fbx_sdk_path = find_fbx_sdk()
    if fbx_sdk_path:
        # Correctly format the path to the x64 folder
        path_to_set = os.path.normpath(os.path.join(fbx_sdk_path, '..', '..', '..'))
        
        # Output the path to a text file for batch file to use
        with open("fbx_sdk_path.txt", "w") as file:
            file.write(path_to_set)
        
        return path_to_set
    else:
        return None

# Main script logic
if __name__ == "__main__":
    fbx_sdk_path = set_fbx_sdk_env()
    if fbx_sdk_path is None:
        print("FBX_SDK_PATH not found!")