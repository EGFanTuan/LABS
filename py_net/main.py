import requests
import json
import os
import re

class BilibiliDownloader:
    def __init__(self):
        self.headers = {
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/129.0.0.0 Safari/537.36 Edg/129.0.0.0",
            "Referer": "https://www.bilibili.com/"
        }
        os.makedirs("./downloads", exist_ok=True)
    
    def get_info_via_bid(self, bid):
        url = "https://api.bilibili.com/x/web-interface/wbi/view"
        params = {"bvid": bid}
        
        try:
            response = requests.get(
                url, 
                params=params,
                headers=self.headers,
                timeout=15
            )
            response.raise_for_status()
            data = response.json()

            # save data
            with open(f"./downloads/{bid}_info.json", "w", encoding="utf-8") as f:
                json.dump(data, f, ensure_ascii=False, indent=4)

            if data["code"] != 0:
                return {}, [], data["message"]
            
            # str for default
            video_infos = {}
            rdata = data["data"]
            video_infos["bvid"] = rdata["bvid"]
            video_infos["videos"] = rdata["videos"] # number
            video_infos["pic"] = rdata["pic"]
            video_infos["title"] = rdata["title"]
            video_infos["desc"] = rdata["desc"]
            video_infos["cid"] = rdata["cid"] # number
            result = []
            for item in rdata["pages"]:
                part_info = {
                    "cid": item["cid"], # number
                    "page": item["page"], # number
                    "part": item["part"], 
                    "first_frame": item.get("first_frame", "")
                }
                result.append(part_info)
            
            return video_infos, result, ""
        
        except Exception as e:
            return {}, [], str(e)
    
    def download_m4s(self, bid, cid, save_path, filename, quality=30280):
        play_url = "https://api.bilibili.com/x/player/wbi/playurl"
        params = {
            "bvid": bid,
            "cid": cid,
            "fnval": 16
        }
        
        try:
            response = requests.get(
                play_url,
                params=params,
                headers=self.headers,
                timeout=15
            )
            response.raise_for_status()
            data = response.json()
            
            if data["code"] != 0:
                return False, data["message"]
            
            audio_streams = data["data"]["dash"]["audio"]
            if not audio_streams:
                return False, "No audio stream found"

            selected_audio = None
            for stream in audio_streams:
                if stream["id"] == quality:
                    selected_audio = stream
                    break
            
            if not selected_audio:
                selected_audio = max(audio_streams, key=lambda x: x["bandwidth"])

            audio_url = selected_audio["baseUrl"]
            audio_response = requests.get(
                audio_url,
                headers=self.headers,
                stream=True,
                timeout=30
            )
            audio_response.raise_for_status()

            os.makedirs(save_path, exist_ok=True)

            clean_filename = re.sub(r'[\\/*?:"<>|]', "", filename)
            save_path = os.path.join(save_path, f"{clean_filename}.m4a")

            with open(save_path, "wb") as f:
                for chunk in audio_response.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)
            
            return True, f"Audio downloaded to: {save_path}"
        
        except Exception as e:
            return False, str(e)

if __name__ == "__main__":
    downloader = BilibiliDownloader()

    idx = 48 # idx, =part - 1
    bvid = "BV1q4FTeBEMj"

    video_infos, parts, message = downloader.get_info_via_bid(bvid)
    print(video_infos)
    print(message)

    if not parts:
        print(f"Error: {message}")
    else:
        print(f"Found {str(video_infos["videos"])} parts:")
        for i, part in enumerate(parts):
            print(f"{i+1}. {part['part']} (CID: {part['cid']})")
        
        idx = min(idx, len(parts) - 1)
        cid = parts[idx]["cid"]
        result, msg = downloader.download_m4s(
            bvid,
            cid,
            "./downloads",
            parts[idx]["part"]
        )
        
        if result:
            print(msg)
        else:
            print(f"Download failed: {msg}")
