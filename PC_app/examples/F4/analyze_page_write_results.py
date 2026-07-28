import sys
import re
import csv

def parse_log(file_path):
    # Output file name based on input file name
    output_csv = "page_write_results.csv"
    
    # Regex to match the address line and extract address and length
    addr_pattern = re.compile(r"address\s*=\s*(0x[0-9a-fA-F]+).*?length:\s*(\d+)")
    # Regex to match the result line and extract the result number/text
    result_pattern = re.compile(r"result\s*=\s*(\d+)")

    data_rows = []
    
    try:
        with open(file_path, 'r') as infile:
            current_address = None
            current_length = None
            
            for line in infile:
                # Check for the address line
                addr_match = addr_pattern.search(line)
                if addr_match:
                    current_address = addr_match.group(1)
                    current_length = addr_match.group(2)
                    continue
                
                # Check for the result line
                res_match = result_pattern.search(line)
                if res_match and current_address is not None:
                    result_val = res_match.group(1)
                    
                    # Append the collected data pair
                    data_rows.append([current_address, current_length, result_val])
                    
                    # Reset variables for the next block
                    current_address = None
                    current_length = None

        # Write the extracted data to the CSV file
        with open(output_csv, 'w', newline='') as outfile:
            writer = csv.writer(outfile)
            # Write headers
            writer.writerow(["address", "length", "result"])
            # Write data
            writer.writerows(data_rows)
            
        print(f"Successfully processed {len(data_rows)} entries. Output saved to '{output_csv}'.")

    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python analyze_page_write_results.py <log file>")
        sys.argv.append("sample.log")
    else:
        parse_log(sys.argv[1])
