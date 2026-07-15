import time
import numpy as np
from perception import CameraNode, PointCloudProcessor
from grasping import AnyGraspDetector, GraspScorer
from robot_control import UR5Controller, DexterousHandController

class GraspingPipeline:
    def __init__(self):
        self.camera = CameraNode()
        self.pc_processor = PointCloudProcessor()
        self.detector = AnyGraspDetector()
        self.scorer = GraspScorer()
        self.arm = UR5Controller()
        self.hand = DexterousHandController()
        
    def execute_grasp(self, object_id):
        print(f"[Pipeline] Starting grasp for object {object_id}")
        
        for attempt in range(1, 4):
            print(f"[Attempt {attempt}] Generating grasp proposals...")
            
            # 1. Perception
            rgbd = self.camera.capture()
            cloud = self.pc_processor.filter_table(rgbd)
            obj_cloud = self.pc_processor.segment_object(cloud, object_id)
            
            # 2. Proposal Generation (6D poses)
            candidates = self.detector.detect_cands(obj_cloud)
            if not candidates:
                print("No candidates found, retrying...")
                continue
                
            # 3. Filtering and Scoring
            valid_cands = []
            for cand in candidates:
                if self.scorer.check_kinematic_feasibility(self.arm, cand) and \
                   not self.scorer.check_collision(cand):
                    
                    # 综合评分: 模型分 + 质心距离分 + 包络率 - 边缘惩罚
                    score = 0.3 * cand.model_score + \
                            self.scorer.score_center_distance(obj_cloud, cand) + \
                            self.scorer.score_enclosure(obj_cloud, cand) - \
                            self.scorer.score_edge_penalty(cand)
                    valid_cands.append((score, cand))
                    
            if not valid_cands:
                print("No valid candidates after filtering.")
                continue
                
            # 取最优抓取
            valid_cands.sort(key=lambda x: x[0], reverse=True)
            best_score, best_cand = valid_cands[0]
            
            # 4. Primitive Mapping
            # 根据位姿的相对方向分配抓取模式
            if attempt == 1:
                primitive = self.hand.map_to_synergy(best_cand, mode='envelop')
            elif attempt == 2:
                primitive = self.hand.map_to_synergy(best_cand, mode='pinch')
            else:
                primitive = self.hand.map_to_synergy(best_cand, mode='scoop') # 兜底策略
                
            # 5. Execution
            print(f"Executing primitive: {primitive.name} with score {best_score:.2f}")
            self.arm.move_to_pose(best_cand.pre_grasp_pose)
            self.hand.open()
            
            self.arm.move_linearly(best_cand.grasp_pose)
            
            # 电流力控闭合
            success = self.hand.close_with_current_feedback(max_current=0.5)
            
            if success:
                self.arm.lift()
                print("[Pipeline] Grasp succeeded!")
                return True
            else:
                self.hand.open()
                self.arm.move_to_home()
                print("[Pipeline] Grasp failed. Slippage detected.")
                
        print("[Pipeline] Task failed after 3 attempts.")
        return False

if __name__ == "__main__":
    pipeline = GraspingPipeline()
    pipeline.execute_grasp(object_id="apple_1")
