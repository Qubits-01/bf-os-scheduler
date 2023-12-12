class BaseNode:
    def __init__(self, value):
        self.value = value
        self.level0 = None
        self.level1 = None
        self.level2 = None
        self.level3 = None

    def connect_to_level(self, slnode):
        if slnode.level == 0:
            self.level0 = slnode
        elif slnode.level == 1:
            self.level1 = slnode
        elif slnode.level == 2:
            self.level2 = slnode
        else:
            self.level3 = slnode

    def return_node(self, level):
        lists = [self.level0, self.level1, self.level2, self.level3]
        
        return lists[level]
    

class SLNode:
    def __init__(self, baseNode, level, next = None, prev = None):
        self.baseNode = baseNode
        self.level = level
        self.next = next
        self.prev = prev

class SkipList:
    def __init__(self):
        rootNode = BaseNode(-1)
        self.level0 = SLNode(rootNode, 0)
        self.level1 = SLNode(rootNode, 1)
        self.level2 = SLNode(rootNode, 2)
        self.level3 = SLNode(rootNode, 3)


        tailNode = BaseNode(float("INF"))
        self.level0.next = SLNode(tailNode, 0)
        self.level1.next = SLNode(tailNode, 1)
        self.level2.next = SLNode(tailNode, 2)
        self.level3.next = SLNode(tailNode, 3)

    def old_insert(self, node, level):
        lists =  [self.level0, self.level1, self.level2, self.level3]

        for l in range(level + 1):
            newNode = SLNode(node, l)
            node.connect_to_level(newNode)
            curr = lists[l]

            while curr.next.baseNode.value < node.value:
                curr = curr.next

            temp = curr.next
            newNode.next = temp
            newNode.prev = curr
            curr.next = newNode
            temp.prev = newNode

    def traverse(self):
        strs = []
        for n in [self.level0, self.level1, self.level2, self.level3]:
            curr = n.next

            s = ""
            while (curr.baseNode.value != float("INF")):
                s += f'{curr.baseNode.value} -> '
                curr = curr.next

            strs.append(s)

        return strs
    
    def reverse_traverse(self):
        strs = []
        for n in [self.level0, self.level1, self.level2, self.level3]:
            curr = n

            s = ""
            while (curr.next.baseNode.value != float("INF")):
                curr = curr.next

            while (curr.baseNode.value != -1):
                s += f'{curr.baseNode.value} <- '
                curr = curr.prev
            
            strs.append(s)
        
        return strs
    
    def print_traverse(self):
        for t in self.traverse()[::-1]:
            print(t)

    def print_reverse(self):
        for t in self.reverse_traverse()[::-1]:
            print(t)
    
    def print_lookup(self, value):
        l = 3 # highest level
        curr = self.level3

        while l >= 0:                
            while curr.baseNode.value != float("INF") and curr.baseNode.value < value:
                print(f'{curr.baseNode.value} -> ', end = "")
                curr = curr.next

            if curr != None:
                if curr.baseNode.value == value:
                    print(f'{curr.baseNode.value}')
                    return value

            if l == 0:
                break

            curr = curr.prev
            baseNode = curr.baseNode
            curr = baseNode.return_node(curr.level - 1) # downgrade
            
            l -= 1
        
        print("Not found")
        return -1
    
    def lookup(self, value):
        l = 3 # highest level
        curr = self.level3

        while l >= 0:                
            while curr.baseNode.value != float("INF") and curr.baseNode.value < value:
                curr = curr.next

            if curr != None:
                if curr.baseNode.value == value:
                    return value

            if l == 0:
                break

            curr = curr.prev # backtrack
            baseNode = curr.baseNode
            curr = baseNode.return_node(curr.level - 1) # downgrade
            
            l -= 1

        return -1

    def min_value_lookup(self):
        return -1 if self.level0.next.baseNode.value == float("INF") else self.level0.next.baseNode.value

def create_skip_list(arr):
    s = SkipList()

    for v, l in arr:
        x = BaseNode(v)
        s.old_insert(x, l)

    return s

if __name__ == "__main__":
    s = create_skip_list([(1, 3), (5, 0), (8, 0), (10, 0), (9, 1), (7, 0), (2, 0), (4, 2), (6, 2), (3, 1)])
    s.print_traverse()
    
    #s.print_reverse()
    #for i in range(1, 11):
        #s.print_lookup(i)