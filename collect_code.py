import os

# Имя итогового файла
OUTPUT_FILE = "project_context.txt"

# Папки, которые нужно просканировать (добавь свои, если нужно)
TARGET_FOLDERS = ["res", "src", "include"] 

# Расширения файлов, которые нас интересуют
VALID_EXTENSIONS = {".glsl", ".vert", ".frag", ".comp", ".geom", ".cpp", ".h", ".hpp"}

def collect_project_files():
    total_files = 0
    with open(OUTPUT_FILE, "w", encoding="utf-8") as outfile:
        outfile.write("=== АРХИТЕКТУРА И ИСХОДНЫЙ КОД ПРОЕКТА ===\n\n")
        
        for folder in TARGET_FOLDERS:
            if not os.path.exists(folder):
                continue
                
            for root, _, files in os.walk(folder):
                for file in files:
                    ext = os.path.splitext(file)[1].lower()
                    if ext in VALID_EXTENSIONS:
                        file_path = os.path.join(root, file)
                        
                        # Пишем красивый заголовок с путем к файлу
                        outfile.write(f"\n{'='*50}\n")
                        outfile.write(f"FILE: {file_path}\n")
                        outfile.write(f"{'='*50}\n\n")
                        
                        try:
                            with open(file_path, "r", encoding="utf-8", errors="ignore") as infile:
                                outfile.write(infile.read())
                                outfile.write("\n\n")
                            total_files += 1
                            print(f"[+] Добавлен: {file_path}")
                        except Exception as e:
                            print(f"[-] Ошибка чтения {file_path}: {e}")

    print(f"\nГотово! Собрано файлов: {total_files}. Итоговый файл: {OUTPUT_FILE}")

if __name__ == "__main__":
    collect_project_files()